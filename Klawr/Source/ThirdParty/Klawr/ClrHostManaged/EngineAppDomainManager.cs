//
// The MIT License (MIT)
//
// Copyright (c) 2014 Vadim Macagon
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//

using Klawr.ClrHost.Interfaces;
using Klawr.ClrHost.Managed.SafeHandles;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Linq.Expressions;
using System.Reflection;
using System.Threading;
using System.Runtime.InteropServices;
using Klawr.ClrHost.Managed.Attributes;
using Klawr.ClrHost.Managed.Wrappers;
using Klawr.UnrealEngine;
using System.IO;
using System.Text;
using NativClass = System.String;
using InstanceId = System.Int64;

namespace Klawr.ClrHost.Managed{
    /// <summary>
    /// Manager for engine app domains (that can be unloaded).
    /// </summary>
    public sealed class EngineAppDomainManager : AppDomainManager, IEngineAppDomainManager{
        public struct ScriptObjectInfo{
            public IScriptObject Instance;
            public ScriptObjectInstanceInfo.BeginPlayAction BeginPlay;
            public ScriptObjectInstanceInfo.TickAction Tick;
            public ScriptObjectInstanceInfo.DestroyAction Destroy;
        }

        public struct ScriptComponentInfo{
            public IDisposable Instance;
            public ScriptComponentProxy Proxy;
        }

        private delegate void SetProxyDelegateAction(ref ScriptComponentProxy proxy, Delegate value);

        private struct ScriptComponentProxyMethodInfo{
            public string Name;
            public Type DelegateType;
            public Type[] ParameterTypes;
            public SetProxyDelegateAction BindToProxy;
        }

        private struct ScriptComponentMethodInfo{
            public Type DelegateType;
            public MethodInfo Method;
            public SetProxyDelegateAction BindToProxy;
        }

        private struct ScriptComponentTypeInfo{
            public ConstructorInfo Constructor;
            public ScriptComponentMethodInfo[] Methods;
        }

        // only set for the engine app domain manager
        private Dictionary<NativClass, IntPtr[]> _nativeFunctionPointers = new Dictionary<string, IntPtr[]>();
        // all currently registered script objects
        private Dictionary<InstanceId, ScriptObjectInfo> _scriptObjects = new Dictionary<long, ScriptObjectInfo>();
        // identifier of the most recently registered ScriptObject instance
        private long _lastScriptObjectID = 0;
        // cache of previously created script object types
        private Dictionary<string /*Full Type Name*/, Type> _scriptObjectTypeCache = new Dictionary<string, Type>();

        private ScriptComponentProxyMethodInfo[] _scriptComponentProxyMethods;
        // all currently registered script components
        private Dictionary<long /*Instance ID*/, ScriptComponentInfo> _scriptComponents = new Dictionary<long, ScriptComponentInfo>();
        // cache of previously created script component types
        private Dictionary<string /*Full Type Name*/, ScriptComponentTypeInfo> _scriptComponentTypeCache = new Dictionary<string, ScriptComponentTypeInfo>();

        // NOTE: the base implementation of this method does nothing, so no need to call it
        public override void InitializeNewDomain(AppDomainSetup appDomainInfo){
            // register the custom domain manager with the unmanaged host
            InitializationFlags = AppDomainManagerInitializationOptions.RegisterWithHost;
        }

        public void SetNativeFunctionPointers(string nativeClassName, long[] functionPointers){
            // the function pointers are passed in as long to avoid pointer truncation on a 
            // 64-bit platform when this method is called via COM, but to actually use them
            // they need to be converted to IntPtr
            var unboxedFunctionPointers = new IntPtr[functionPointers.Length];
            for (var i = 0; i < functionPointers.Length; ++i){
                unboxedFunctionPointers[i] = (IntPtr) (functionPointers[i]);
            }
            _nativeFunctionPointers[nativeClassName] = unboxedFunctionPointers;
        }

        public IntPtr[] GetNativeFunctionPointers(string nativeClassName){
            return _nativeFunctionPointers[nativeClassName];
        }

        public void LoadUnrealEngineWrapperAssembly(){
            // TODO: this may not be the best place to call it
            CacheScriptComponentProxyInfo();

            var wrapperAssembly = new AssemblyName{Name = GlobalStrings.KlawrUnrealEngineNamespace};
            Assembly.Load(wrapperAssembly);
        }

        public bool LoadAssembly(string assemblyName){
            var assembly = new AssemblyName{Name = assemblyName};

            try{
                Assembly.Load(assembly);
            } catch (Exception except){
                Console.WriteLine(except.ToString());
                return false;
            }

            return true;
        }

        public bool CreateScriptObject(string className, IntPtr nativeObject, ref ScriptObjectInstanceInfo info){
            var objType = FindScriptObjectTypeByName(className);
            if (objType != null){
                var constructor = objType.GetConstructor(new[]{typeof(long), typeof(UObjectHandle)});
                if (constructor != null){
                    var instanceID = GenerateScriptObjectID();
                    // The handle created here is set not to release the native object when the
                    // handle is disposed because that object is actually the owner of the script 
                    // object created here, and no additional references are created to owners at
                    // the moment so there is no reference to remove.
                    var objectHandle = new UObjectHandle(nativeObject, false);
                    var obj = (IScriptObject) constructor.Invoke(
                        new object[]{instanceID, objectHandle}
                    );
                    var objInfo = RegisterScriptObject(obj);
                    info.InstanceID = instanceID;
                    info.BeginPlay = objInfo.BeginPlay;
                    info.Tick = objInfo.Tick;
                    info.Destroy = objInfo.Destroy;
                    return true;
                }
            }
            // TODO: log an error
            return false;
        }

        public void DestroyScriptObject(long scriptObjectInstanceID){
            var instance = UnregisterScriptObject(scriptObjectInstanceID);
            instance.Dispose();
        }
        
        /// <summary>
        /// Note that the identifier returned by this method is only unique amongst all ScriptObject 
        /// instances registered with this manager instance. The returned identifier can be used to 
        /// uniquely identify a ScriptObject instance within the app domain it was created in.
        /// </summary>
        /// <returns>Unique identifier.</returns>
        public long GenerateScriptObjectID(){
            return Interlocked.Increment(ref _lastScriptObjectID);
        }

        /// <summary>
        /// Register the given IScriptObject instance with the manager.
        /// 
        /// The manager will keep a reference to the given object until the object is unregistered.
        /// </summary>
        /// <param name="scriptObject"></param>
        /// <returns></returns>
        public ScriptObjectInfo RegisterScriptObject(IScriptObject scriptObject){
            ScriptObjectInfo info;
            info.Instance = scriptObject;
            info.BeginPlay = scriptObject.BeginPlay;
            info.Tick = scriptObject.Tick;
            info.Destroy = scriptObject.Destroy;
            _scriptObjects.Add(scriptObject.InstanceID, info);
            return info;
        }

        /// <summary>
        /// Unregister a IScriptObject that was previously registered with the manager.
        /// 
        /// The manager will remove the reference it previously held to the object.
        /// </summary>
        /// <param name="scriptObjectInstanceID">ID of a registered IScriptObject instance.</param>
        /// <returns>The script object matching the given ID.</returns>
        public IScriptObject UnregisterScriptObject(long scriptObjectInstanceID)
        {
            var instance = _scriptObjects[scriptObjectInstanceID].Instance;
            _scriptObjects.Remove(scriptObjectInstanceID);
            return instance;
        }

        /// <summary>
        /// Search all loaded (non-dynamic) assemblies for a Type matching the given name and
        /// implementing the IScriptObject interface.
        /// </summary>
        /// <param name="typeName">The full name of a type (including the namespace).</param>
        /// <returns>Matching Type instance, or null if no match was found.</returns>
        private Type FindScriptObjectTypeByName(string typeName){
            Type objType = null;
            if (!_scriptObjectTypeCache.TryGetValue(typeName, out objType)){
                objType = AppDomain.CurrentDomain.GetAssemblies()
                    .Where(assembly => !assembly.IsDynamic)
                    .SelectMany(assembly => assembly.GetTypes())
                    .FirstOrDefault(
                        t => t.FullName.Equals(typeName) 
                            && t.GetInterfaces().Contains(typeof(IScriptObject))
                    );

                if (objType != null){
                    // cache the result to speed up future searches
                    _scriptObjectTypeCache.Add(typeName, objType);
                }
            }
            return objType;
        }

        /// <summary>
        /// Search all loaded (non-dynamic) assemblies for a Type matching the given name.
        /// </summary>
        /// <param name="typeName">The full name of a type (including the namespace).</param>
        /// <returns>Matching Type instance, or null if no match was found.</returns>
        private static Type FindTypeByName(string typeName){
            return AppDomain.CurrentDomain.GetAssemblies()
                            .Where(assembly => !assembly.IsDynamic)
                            .SelectMany(assembly => assembly.GetTypes())
                            .FirstOrDefault(t => t.FullName.Equals(typeName));
        }

        public void BindUtils(ref ObjectUtilsProxy objectUtilsProxy, ref LogUtilsProxy logUtilsProxy, ref ArrayUtilsProxy arrayUtilsProxy){
            new ObjectUtils(ref objectUtilsProxy);
            new LogUtils(ref logUtilsProxy);
            // redirect output to the UE console and log file (needs LogUtils)
            Console.SetOut(new UELogWriter());
            new ArrayUtils(ref arrayUtilsProxy);
        }

        public bool CreateScriptComponent(string className, IntPtr nativeComponent, ref ScriptComponentProxy proxy){
            ScriptComponentTypeInfo componentTypeInfo;
            if (FindScriptComponentTypeByName(className, out componentTypeInfo)){
                if (componentTypeInfo.Constructor != null){
                    var instanceID = GenerateScriptObjectID();
                    // The handle created here is set not to release the native object when the
                    // handle is disposed because that object is actually the owner of the script 
                    // object created here, and no additional references are created to owners at
                    // the moment so there is no reference to remove.
                    var objectHandle = new UObjectHandle(nativeComponent, false);
                    var component = (IDisposable) componentTypeInfo.Constructor.Invoke(
                                                                       new object[]{instanceID, objectHandle}
                                                                   );
                    // initialize the script component proxy
                    proxy.InstanceID = instanceID;
                    foreach (var methodInfo in componentTypeInfo.Methods){
                        methodInfo.BindToProxy(
                            ref proxy,
                            Delegate.CreateDelegate(
                                methodInfo.DelegateType, component, methodInfo.Method
                            )
                        );
                    }
                    // keep anything that may be called from native code alive
                    RegisterScriptComponent(instanceID, component, proxy);
                    return true;
                }
            }
            // TODO: log an error
            return false;
        }

        public void DestroyScriptComponent(long instanceID){
            var instance = UnregisterScriptComponent(instanceID);
            instance.Dispose();
        }

        private void RegisterScriptComponent(long instanceID, IDisposable scriptComponent, ScriptComponentProxy proxy){
            ScriptComponentInfo componentInfo;
            componentInfo.Instance = scriptComponent;
            componentInfo.Proxy = proxy;
            _scriptComponents.Add(instanceID, componentInfo);
        }

        private IDisposable UnregisterScriptComponent(long instanceID)
        {
            var instance = _scriptComponents[instanceID].Instance;
            _scriptComponents.Remove(instanceID);
            return instance;
        }

        /// <summary>
        /// Search all loaded (non-dynamic) assemblies for a Type matching the given name, the Type
        /// should be derived from UKlawrScriptComponent (but this is not enforced yet).
        /// </summary>
        /// <param name="typeName">The full name of a type (including the namespace).</param>
        /// <param name="componentTypeInfo">Structure to be filled in with type information.</param>
        /// <returns>true if type information matching the given type name was found, false otherwise</returns>
        private bool FindScriptComponentTypeByName(string typeName, out ScriptComponentTypeInfo componentTypeInfo){
            if (!_scriptComponentTypeCache.TryGetValue(typeName, out componentTypeInfo)){
                var componentType = FindTypeByName(typeName);
                if (componentType != null){
                    componentTypeInfo = GetComponentTypeInfo(componentType);
                    // cache the result to speed up future searches
                    _scriptComponentTypeCache.Add(typeName, componentTypeInfo);
                } else{
                    return false;
                }
            }
            return true;
        }

        private ScriptComponentTypeInfo GetComponentTypeInfo(Type componentType){
            ScriptComponentTypeInfo typeInfo;
            typeInfo.Constructor = componentType.GetConstructor(new[]{typeof(long), typeof(UObjectHandle)});

            // Currently all script component classes must directly subclass UKlawScriptComponent, 
            // they cannot subclass another script component. The virtual methods in 
            // UKlawScriptComponent have default implementations that do nothing, these can be 
            // overridden in subclasses, but if they're not then they should never be called from 
            // native code to avoid a pointless native/managed transition. BindingFlags.DeclaredOnly
            // is sufficient to detect if a method has been overridden in a subclass for now, but
            // this will have to be revisited if script classes are allowed to subclass other
            // script classes in the future.
            BindingFlags bindingFlags = BindingFlags.DeclaredOnly
                                        | BindingFlags.Instance
                                        | BindingFlags.Public
                                        | BindingFlags.NonPublic;

            var implementedMethodList = new List<ScriptComponentMethodInfo>();
            foreach (var proxyMethod in _scriptComponentProxyMethods){
                // FIXME: catch and log exceptions
                var method = componentType.GetMethod(proxyMethod.Name, bindingFlags, null, proxyMethod.ParameterTypes, null);
                if (method != null){
                    ScriptComponentMethodInfo methodInfo;
                    methodInfo.DelegateType = proxyMethod.DelegateType;
                    methodInfo.Method = method;
                    methodInfo.BindToProxy = proxyMethod.BindToProxy;
                    implementedMethodList.Add(methodInfo);
                }
            }
            typeInfo.Methods = implementedMethodList.ToArray();
            return typeInfo;
        }

        private void CacheScriptComponentProxyInfo(){
            // grab all the public delegate instance fields
            var fields = typeof(ScriptComponentProxy)
                .GetFields(BindingFlags.Public | BindingFlags.Instance)
                .Where(field => field.FieldType.IsSubclassOf(typeof(Delegate)))
                .ToArray();

            // store the info that will be useful later on
            _scriptComponentProxyMethods = new ScriptComponentProxyMethodInfo[fields.Length];
            for (int i = 0; i < fields.Length; i++){
                var fieldInfo = fields[i];
                var methodInfo = fieldInfo.FieldType.GetMethod("Invoke");
                var parameters = methodInfo.GetParameters();

                ScriptComponentProxyMethodInfo info;
                info.Name = fieldInfo.Name;
                info.DelegateType = fieldInfo.FieldType;
                info.ParameterTypes = new Type[parameters.Length];
                for (int j = 0; j < parameters.Length; j++){
                    info.ParameterTypes[j] = parameters[j].ParameterType;
                }
                info.BindToProxy = BuildProxyDelegateSetter(fieldInfo);
                _scriptComponentProxyMethods[i] = info;
            }
        }

        /// <summary>
        /// Build a delegate that sets one of the delegate fields in ScriptComponentProxy.
        /// 
        /// Setting a field on struct could've been done via reflection, but this is faster, and 
        /// works without boxing/unboxing or using undocumented features like __makeref().
        /// </summary>
        /// <param name="field">The field whose value the delegate should set when invoked.</param>
        /// <returns>A delegate.</returns>
        private SetProxyDelegateAction BuildProxyDelegateSetter(FieldInfo field){
            var proxyExpr = Expression.Parameter(typeof(ScriptComponentProxy).MakeByRefType());
            var valueExpr = Expression.Parameter(typeof(Delegate), "value");
            var lambdaExpr = Expression.Lambda<SetProxyDelegateAction>(
                Expression.Assign(
                    Expression.Field(proxyExpr, field),
                    Expression.Convert(valueExpr, field.FieldType)
                ),
                proxyExpr, valueExpr
            );
            return lambdaExpr.Compile();
        }

        public string[] GetScriptComponentTypes(){
            // this type is defined in the UE4 wrappers assembly
            var scriptComponentType = FindTypeByName($"{GlobalStrings.KlawrUnrealEngineNamespace}.UKlawrScriptComponent");
            if (scriptComponentType == null){
                return new string[]{};
            }

            return AppDomain.CurrentDomain.GetAssemblies()
                            .Where(assembly => !assembly.IsDynamic)
                            .SelectMany(assembly => assembly.GetTypes())
                            .Where(t => t.IsSubclassOf(scriptComponentType))
                            .Select(t => t.FullName)
                            .ToArray();
        }

        public string[] GetScriptComponentPropertyNames(string componentName)
        {
            var scriptComponentType = FindTypeByName(componentName);
            if (scriptComponentType == null)
            {
                LogUtils.LogError("Component " + componentName + " NOT FOUND!");
                return new string[0];
            }
            return scriptComponentType.GetProperties(BindingFlags.Public | BindingFlags.Instance).Where(property => property.GetCustomAttributes<UPROPERTYAttribute>(true).Any()).Select(x => x.Name).ToArray();
        }

        public string[] GetScriptComponentPropertyMetadata(string componentName, string propertyName)
        {
            var scriptComponentType = FindTypeByName(componentName);
            if (scriptComponentType == null)
            {
                LogUtils.LogError("Component " + componentName + " NOT FOUND!");
                return new string[0];
            }
            PropertyInfo pi = scriptComponentType.GetProperty(propertyName);
            if (pi == null)
            {
                LogUtils.LogError("Component " + componentName + " Property " + propertyName + " NOT FOUND!");
                return new string[0];
            }

            // Don't inherit for Meta-Data
            UPROPERTYAttribute upa = pi.GetCustomAttribute<UPROPERTYAttribute>(false);
            if (upa != null)
            {
                return upa.GetMetas();
            }
            return new string[0];
        }

        public bool GetScriptComponentPropertyIsAdvancedDisplay(string componentName, string propertyName)
        {
            var scriptComponentType = FindTypeByName(componentName);
            if (scriptComponentType == null)
            {
                LogUtils.LogError("Component " + componentName + " NOT FOUND!");
                return false;
            }
            PropertyInfo pi = scriptComponentType.GetProperty(propertyName);
            if (pi == null)
            {
                LogUtils.LogError("Component " + componentName + " Property " + propertyName + " NOT FOUND!");
                return false;
            }

            // Don't inherit for Meta-Data
            UPROPERTYAttribute upa = pi.GetCustomAttribute<UPROPERTYAttribute>(false);
            if (upa != null)
            {
                return upa.AdvancedDisplay;
            }
            return false;
        }

        public bool GetScriptComponentPropertyIsSaveGame(string componentName, string propertyName)
        {
            var scriptComponentType = FindTypeByName(componentName);
            if (scriptComponentType == null)
            {
                LogUtils.LogError("Component " + componentName + " NOT FOUND!");
                return false;
            }
            PropertyInfo pi = scriptComponentType.GetProperty(propertyName);
            if (pi == null)
            {
                LogUtils.LogError("Component " + componentName + " Property " + propertyName + " NOT FOUND!");
                return false;
            }

            // Don't inherit for Meta-Data
            UPROPERTYAttribute upa = pi.GetCustomAttribute<UPROPERTYAttribute>(false);
            if (upa != null)
            {
                return upa.SaveGame;
            }
            return false;
        }
        public string[] GetScriptComponentFunctionNames(string componentName)
        {
            var scriptComponentType = FindTypeByName(componentName);
            if (scriptComponentType == null)
            {
                LogUtils.LogError("Component " + componentName + " NOT FOUND!");
                return new string[0];
            }
            return scriptComponentType.GetMethods(BindingFlags.Public | BindingFlags.Instance).Where(function => function.GetCustomAttributes<UFUNCTIONAttribute>(true).Any()).Select(x => x.Name).ToArray();
        }

        public int GetScriptComponentFunctionReturnType(string componentName, string functionName)
        {
            var scriptComponentType = FindTypeByName(componentName);
            if (scriptComponentType == null)
            {
                LogUtils.LogError("Component " + componentName + " NOT FOUND!");
                return -2;
            }

            MethodInfo mi = scriptComponentType.GetMethod(functionName);
            if (mi.ReturnType == typeof(float))
            { return 0; }
            else if (mi.ReturnType == typeof(int))
            { return 1; }
            else if (mi.ReturnType == typeof(bool))
            { return 2; }
            else if (mi.ReturnType == typeof(string))
            { return 3; }
            else if (mi.ReturnType.IsSubclassOf(typeof(UObject)))
            { return 4; }
            // Unknown type??
            return -1;
        }

        public string[] GetScriptComponentFunctionParameterNames(string componentName, string functionName)
        {
            var scriptComponentType = FindTypeByName(componentName);
            if (scriptComponentType == null)
            {
                LogUtils.LogError("Component " + componentName + " NOT FOUND!");
                return new string[0];
            }

            MethodInfo mi = scriptComponentType.GetMethod(functionName);
            return mi.GetParameters().Select(x => x.Name).ToArray();
        }

        public int GetScriptComponentFunctionParameterType(string componentName, string functionName, int parameterCount)
        {
            var scriptComponentType = FindTypeByName(componentName);
            if (scriptComponentType == null)
            {
                LogUtils.LogError("Component " + componentName + " NOT FOUND!");
                return -2;
            }

            MethodInfo mi = scriptComponentType.GetMethod(functionName);
            Type parameterType;
            if (parameterCount == -1)
            {
                parameterType = mi.ReturnType;
            }
            else
            {
                parameterType = mi.GetParameters()[parameterCount].ParameterType;
            }
            if (parameterType == typeof(float))
            { return 0; }
            else if (parameterType == typeof(int))
            { return 1; }
            else if (parameterType == typeof(bool))
            { return 2; }
            else if (parameterType == typeof(string))
            { return 3; }
            else if (parameterType.IsSubclassOf(typeof(UObject)))
            { return 4; }
            else if (parameterType == typeof(void))
            { return 5; }
            return -1;
        }

        public string GetScriptComponentFunctionParameterTypeObjectClass(string componentName, string functionName, int parameterCount)
        {
            var scriptComponentType = FindTypeByName(componentName);
            if (scriptComponentType == null)
            {
                LogUtils.LogError("Component " + componentName + " NOT FOUND!");
                return "";
            }

            MethodInfo mi = scriptComponentType.GetMethod(functionName);
            Type parameterType = mi.GetParameters()[parameterCount].ParameterType;

            bool removeFirstChar = parameterType.GetCustomAttribute<ConvertClassNameAttribute>() != null;
            if (removeFirstChar)
            {
                return parameterType.Name.Substring(1);
            }
            return parameterType.Name;
        }

        public int GetScriptComponentPropertyType(string componentName, string propertyName)
        {
            PropertyInfo pi = FindTypeByName(componentName).GetProperty(propertyName);
            if (pi.PropertyType == typeof(float))
            { return 0; }
            else if (pi.PropertyType == typeof(int))
            { return 1; }
            else if (pi.PropertyType == typeof(bool))
            { return 2; }
            else if (pi.PropertyType == typeof(string))
            { return 3; }
            else if (pi.PropertyType.IsSubclassOf(typeof(UObject)))
            { return 4; }

            // Unknown type??
            return -1;
        }


        public string GetScriptComponentPropertyClassType(string componentName, string propertyName)
        {
            PropertyInfo pi = FindTypeByName(componentName).GetProperty(propertyName);
            if (pi.PropertyType.IsSubclassOf(typeof(UObject)))
            {
                Type tp = pi.PropertyType;
                bool removeFirstChar = pi.PropertyType.GetCustomAttribute<ConvertClassNameAttribute>() != null;
                if (removeFirstChar)
                {
                    return tp.Name.Substring(1);
                }
                return tp.Name;
            }
            return "";
        }

        public void SetFloat(long instanceID, string propertyName, float value)
        {
            Type instanceType = _scriptComponents[instanceID].Instance.GetType();
            instanceType.GetProperty(propertyName).SetValue(_scriptComponents[instanceID].Instance, value);
        }

        public void SetInt(long instanceID, string propertyName, int value)
        {
            Type instanceType = _scriptComponents[instanceID].Instance.GetType();
            instanceType.GetProperty(propertyName).SetValue(_scriptComponents[instanceID].Instance, value);
        }

        public void SetBool(long instanceID, string propertyName, bool value)
        {
            Type instanceType = _scriptComponents[instanceID].Instance.GetType();
            instanceType.GetProperty(propertyName).SetValue(_scriptComponents[instanceID].Instance, value);
        }

        public void SetStr(long instanceID, string propertyName, string value)
        {
            Type instanceType = _scriptComponents[instanceID].Instance.GetType();
            instanceType.GetProperty(propertyName).SetValue(_scriptComponents[instanceID].Instance, value);
        }

        public void SetObj(long instanceID, string propertyName, IntPtr value)
        {
            Type instanceType = _scriptComponents[instanceID].Instance.GetType();
            var nativeObject = new UObjectHandle(value, false);
            PropertyInfo pi = instanceType.GetProperty(propertyName);
            Type propType = pi.PropertyType;
            var constructor = propType.GetConstructor(new Type[] { typeof(UObjectHandle) });
            var idisp = constructor.Invoke(new object[] { nativeObject });
            pi.SetValue(_scriptComponents[instanceID].Instance, idisp);
        }

        public float GetFloat(long instanceID, string propertyName)
        {
            Type instanceType = _scriptComponents[instanceID].Instance.GetType();
            return (float)instanceType.GetProperty(propertyName).GetValue(_scriptComponents[instanceID].Instance);
        }

        public int GetInt(long instanceID, string propertyName)
        {
            Type instanceType = _scriptComponents[instanceID].Instance.GetType();
            return (int)instanceType.GetProperty(propertyName).GetValue(_scriptComponents[instanceID].Instance);
        }

        public bool GetBool(long instanceID, string propertyName)
        {
            Type instanceType = _scriptComponents[instanceID].Instance.GetType();
            return (bool)instanceType.GetProperty(propertyName).GetValue(_scriptComponents[instanceID].Instance);
        }

        public string GetStr(long instanceID, string propertyName)
        {
            Type instanceType = _scriptComponents[instanceID].Instance.GetType();
            return (string)instanceType.GetProperty(propertyName).GetValue(_scriptComponents[instanceID].Instance);
        }

        public IntPtr GetObj(long instanceID, string propertyName)
        {
            Type instanceType = _scriptComponents[instanceID].Instance.GetType();
            PropertyInfo pi = instanceType.GetProperty(propertyName);
            Type propType = pi.PropertyType;
            UObject uobject = (UObject)instanceType.GetProperty(propertyName).GetValue(_scriptComponents[instanceID].Instance);
            if (uobject == null)
            {
                return IntPtr.Zero;
            }
            return uobject.NativeObject.Handle;
        }

        private T DoCSFunctionCall<T>(long instanceID, string functionName, object[] args)
        {
            Type instanceType = _scriptComponents[instanceID].Instance.GetType();
            MethodInfo mi = instanceType.GetMethod(functionName);

            try
            {
                // Translate args
                for (int i = 0; i < args.Length; i++)
                {
                    if (args[i] is IntPtr)
                    {
                        var ptr = (IntPtr)args[i];
                        if (ptr == IntPtr.Zero)
                        {
                            args[i] = null;
                        }
                        else
                        {
                            Type parameterType = mi.GetParameters()[i].ParameterType;
                            var nativeObject = new UObjectHandle(ptr, false);
                            var constructor = parameterType.GetConstructor(new Type[] { typeof(UObjectHandle) });
                            var idisp = constructor.Invoke(new object[] { nativeObject });
                            args[i] = idisp;
                        }
                    }
                }

                // Perform call
                return (T)mi.Invoke(_scriptComponents[instanceID].Instance, args);
            }
            catch (Exception ee)
            {
                LogUtils.Log(ee.StackTrace + "\r\n" + ee.Message);
                return default(T);
            }
        }

        public float CallCSFunctionFloat(long instanceID, string functionName, object[] args)
        {
            return DoCSFunctionCall<float>(instanceID, functionName, args);
        }
        public int CallCSFunctionInt(long instanceID, string functionName, object[] args)
        {
            return DoCSFunctionCall<int>(instanceID, functionName, args);
        }
        public bool CallCSFunctionBool(long instanceID, string functionName, object[] args)
        {
            return DoCSFunctionCall<bool>(instanceID, functionName, args);
        }
        public string CallCSFunctionString(long instanceID, string functionName, object[] args)
        {
            return DoCSFunctionCall<string>(instanceID, functionName, args);
        }
        public IntPtr CallCSFunctionObject(long instanceID, string functionName, object[] args)
        {
            UObject obj = DoCSFunctionCall<UObject>(instanceID, functionName, args);
            if (obj == null) return IntPtr.Zero;
            if (obj.NativeObject == null) return IntPtr.Zero;
            return obj.NativeObject.Handle;
        }
        public void CallCSFunctionVoid(long instanceID, string functionName, object[] args)
        {
            DoCSFunctionCall<object>(instanceID, functionName, args);
        }

        private string Quoted(string str)
        {
            return "\"" + str + "\"";
        }

        public string GetAssemblyInfo()
        {
            // this type is defined in the UE4 wrappers assembly
            var scriptComponentType = FindTypeByName("Klawr.UnrealEngine.UKlawrScriptComponent");
            if (scriptComponentType == null)
            {
                return "";
            }
            StringBuilder result = new StringBuilder();
            try
            {
                result.Append("{ " + Quoted("classInfos") + ": [ ");
                ExportClassInfos(result);
                result.Append("], " + Quoted("enumInfos") + ": [ ");
                ExportEnumInfos(result);
                result.Append("]}");
                

                
            }
            catch (Exception ex)
            {
                result.Append(ex.StackTrace + " " + ex.Message);
            }

            string str = result.ToString();

            File.WriteAllText("assemblyInfo.json", str);

            return str;
        }

        private void ExportClassInfos(StringBuilder result)
        {
            var scriptComponentType = FindTypeByName("Klawr.UnrealEngine.UKlawrScriptComponent");
            bool firstClass = true;
            foreach (var componentTypeName in AppDomain.CurrentDomain.GetAssemblies()
                .Where(assembly => !assembly.IsDynamic)
                .SelectMany(assembly => assembly.GetTypes())
                .Where(t => t.IsSubclassOf(scriptComponentType))
                .Select(t => t.FullName))
            {
                Type componentType = FindTypeByName(componentTypeName);
                if (!firstClass)
                {
                    result.Append(",");
                }
                firstClass = false;
                result.Append("{" + Quoted("name") + ": " + Quoted(componentType.FullName) + "," + Quoted("methodInfos") + ": [ ");
                // Get method infos
                bool first = true;
                foreach (MethodInfo methodInfo in componentType.GetMethods(BindingFlags.Public | BindingFlags.Instance).Where(function => function.GetCustomAttributes<UFUNCTIONAttribute>(true).Any()))
                {
                    if (!first)
                    {
                        result.Append(",");
                    }
                    first = false;
                    result.Append("{" + Quoted("name") + ": " + Quoted(methodInfo.Name) + "," + Quoted("returnType") + ": " + TranslateReturnType(methodInfo.ReturnType) + ",");
                    result.Append(Quoted("className") + ":" + Quoted(GetClassName(methodInfo.ReturnType)) + ",");
                    result.Append(Quoted("parameters") + ": [");
                    bool firstParam = true;
                    foreach (ParameterInfo parameterInfo in methodInfo.GetParameters())
                    {
                        if (!firstParam)
                        {
                            result.Append(",");
                        }
                        firstParam = false;
                        result.Append("{" + Quoted("name") + ": " + Quoted(parameterInfo.Name) + "," + Quoted("typeId") + ": " + TranslateReturnType(parameterInfo.ParameterType) + "," + Quoted("className") + ":");
                        result.Append(Quoted(GetClassName(parameterInfo.ParameterType)) + "," + Quoted("metaData") + ":[]");
                        result.Append("}");
                    }

                    result.Append("],");
                    result.Append(Quoted("metaData") + ":[");
                    bool firstMeta = true;
                    // Can not be NULL
                    UFUNCTIONAttribute ufa = (UFUNCTIONAttribute)methodInfo.GetCustomAttribute(typeof(UFUNCTIONAttribute));
                    string[] tempMetas = ufa.GetMetas();
                    for (int i = 0; i < tempMetas.Length; i += 2)
                    {
                        if (!firstMeta)
                        {
                            result.Append(",");
                        }
                        firstMeta = false;
                        result.Append("{" + Quoted("metaKey") + ":" + Quoted(tempMetas[i]) + ",");
                        result.Append(Quoted("metaValue") + ":" + Quoted(tempMetas[i + 1]) + "}");
                    }
                    result.Append("]");
                    result.Append("}");
                }

                result.Append("]," + Quoted("propertyInfos") + ": [");
                bool firstProperty = true;
                // Get property infos
                foreach (PropertyInfo propertyInfo in componentType.GetProperties(BindingFlags.Public | BindingFlags.Instance).Where(property => property.GetCustomAttributes<UPROPERTYAttribute>(true).Any()))
                {
                    if (!firstProperty)
                    {
                        result.Append(",");
                    }
                    firstProperty = false;
                    result.Append("{" + Quoted("name") + ": " + Quoted(propertyInfo.Name) + "," + Quoted("typeId") + ": " + TranslateReturnType(propertyInfo.PropertyType) + "," + Quoted("className") + ":");

                    result.Append(Quoted(GetClassName(propertyInfo.PropertyType)) + ",");
                    result.Append(Quoted("metaData") + ":[");
                    bool firstMeta = true;
                    UPROPERTYAttribute upa = propertyInfo.GetCustomAttribute<UPROPERTYAttribute>();
                    string[] tempMetas = upa.GetMetas();
                    for (int i = 0; i < tempMetas.Length; i += 2)
                    {
                        if (!firstMeta)
                        {
                            result.Append(",");
                        }
                        firstMeta = false;
                        result.Append("{" + Quoted("metaKey") + ":" + Quoted(tempMetas[i]) + ",");
                        result.Append(Quoted("metaValue") + ":" + Quoted(tempMetas[i + 1]) + "}");
                    }
                    result.Append("]");

                    result.Append("}");
                }
                result.Append("]}");
            }
        }

        private void ExportEnumInfos(StringBuilder result)
        {
            bool firstEnum = true;
            foreach (var enumType in AppDomain.CurrentDomain.GetAssemblies()
              .Where(assembly => !assembly.IsDynamic)
              .SelectMany(assembly => assembly.GetTypes())
              .Where(t => t.IsEnum && t.GetCustomAttribute<UENUMAttribute>() != null)
            )
            {
                if (!firstEnum)
                {
                    result.Append(", ");
                }
                firstEnum = false;
                result.Append("{");
                result.Append(Quoted("name"));
                result.Append(": ");
                result.Append(Quoted(enumType.FullName));
                result.Append(", ");
                result.Append(Quoted("values"));
                result.Append(": [ ");
                int[] values = Enum.GetValues(enumType) as int[];
                bool firstName = true;
                foreach (int value in values)
                {
                    string name = Enum.GetName(enumType, value);
                    if (!firstName)
                    {
                        result.Append(", ");
                    }
                    firstName = false;
                    result.Append("{");
                    result.Append(Quoted("key"));
                    result.Append(": ");
                    result.Append(Quoted(name));
                    result.Append(", ");
                    result.Append(Quoted("value"));
                    result.Append(": ");
                    result.Append(value);
                    result.Append("}");
                }
                result.Append(" ]}");
            }
        }

        private string GetClassName(Type type)
        {
            string typeName = "";
            if ((type == typeof(UObject)) || (type.IsSubclassOf(typeof(UObject))))
            {
                typeName = type.Name;
                if (type.GetCustomAttribute<ConvertClassNameAttribute>() != null)
                {
                    typeName = typeName.Substring(1);
                }
            }
            return typeName;
        }

        private int TranslateReturnType(Type inputType)
        {
            if (inputType == typeof(void))
            {
                return (int)ParameterTypeTranslation.ParametertypeVoid;
            }
            else if (inputType == typeof(int))
            {
                return (int)ParameterTypeTranslation.ParametertypeInt;
            }
            else if (inputType == typeof(float))
            {
                return (int)ParameterTypeTranslation.ParametertypeFloat;
            }
            else if (inputType == typeof(bool))
            {
                return (int)ParameterTypeTranslation.ParametertypeBool;
            }
            else if (inputType == typeof(string))
            {
                return (int)ParameterTypeTranslation.ParametertypeString;
            }
            else if ((inputType == typeof(UObject)) || (inputType.IsSubclassOf(typeof(UObject))))
            {
                return (int)ParameterTypeTranslation.ParametertypeObject;
            }

            // Nothing of the above applied, return negative
            return -1;
        }
    }
}
