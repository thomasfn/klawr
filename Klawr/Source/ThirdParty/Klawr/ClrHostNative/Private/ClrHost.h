//-------------------------------------------------------------------------------
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
//-------------------------------------------------------------------------------
#pragma once

#include "KlawrClrHostPCH.h"
#include "KlawrClrHost.h"
#include <comdef.h> // for _COM_SMARTPTR_TYPEDEF
#include <map>
#include <string>

_COM_SMARTPTR_TYPEDEF(ICLRRuntimeHost, IID_ICLRRuntimeHost); // for ICLRRuntimeHostPtr

namespace Klawr {

/**
 * @brief The implementation of the public IClrHost interface.
 */
class ClrHost : public IClrHost
{
public: // IClrHost interface
	virtual bool Startup(const TCHAR* engineAppDomainAppBase, const TCHAR* gameScriptsAssemblyName) override;
	virtual bool CreateEngineAppDomain(int& outAppDomainID) override;
	virtual bool InitEngineAppDomain(int appDomainID, const NativeUtils& nativeUtils) override;
	virtual bool DestroyEngineAppDomain(int appDomainID);
	virtual void Shutdown() override;

	virtual void AddClass(const TCHAR* className, void** wrapperFunctions, int numFunctions) override
	{
		_classWrappers[className] = { wrapperFunctions, numFunctions };
	}

	virtual bool CreateScriptObject(
		int appDomainID, const TCHAR* className, class UObject* owner, ScriptObjectInstanceInfo& info
	) override;

	virtual void DestroyScriptObject(int appDomainID, __int64 instanceID) override;

	virtual bool CreateScriptComponent(
		int appDomainID, const TCHAR* className, class UObject* nativeComponent, ScriptComponentProxy& proxy
	) override;

	virtual void DestroyScriptComponent(int appDomainID, __int64 instanceID) override;

	virtual void GetScriptComponentTypes(int appDomainID, std::vector<tstring>& types) const override;

	virtual bool GetScriptComponentPropertyIsAdvancedDisplay(int appDomainID, const TCHAR* typeName, const TCHAR* propertyName) const override;
	virtual bool GetScriptComponentPropertyIsSaveGame(int appDomainID, const TCHAR* typeName, const TCHAR* propertyName) const override;

	virtual void SetFloat(const int appDomainID, const __int64 instanceID, const TCHAR* propertyName, float value) const override;
	virtual void SetInt(const int appDomainID, const __int64 instanceID, const TCHAR* propertyName, int value) const override;
	virtual void SetBool(const int appDomainID, const __int64 instanceID, const TCHAR* propertyName, bool value) const override;
	virtual void SetStr(const int appDomainID, const __int64 instanceID, const TCHAR* propertyName, const TCHAR* value) const override;
	virtual void SetObj(const int appDomainID, const __int64 instanceID, const TCHAR* propertyName, UObject* value) const override;

	virtual float GetFloat(const int appDomainID, const __int64 instanceID, const TCHAR* propertyName) const override;
	virtual int GetInt(const int appDomainID, const __int64 instanceID, const TCHAR* propertyName) const override;
	virtual bool GetBool(const int appDomainID, const __int64 instanceID, const TCHAR* propertyName) const override;
	virtual const TCHAR* GetStr(const int appDomainID, const __int64 instanceID, const TCHAR* propertyName) const override;
	virtual UObject* GetObj(const int appDomainID, const __int64 instanceID, const TCHAR* propertyName) const override;

	virtual float CallCSFunctionFloat(int appDomainID, __int64 instanceID, const TCHAR* functionName, std::vector<float> floats, std::vector<int> ints, std::vector<bool> bools, std::vector<const TCHAR*> strings, std::vector<LONGLONG> objects) const override;
	virtual int CallCSFunctionInt(int appDomainID, __int64 instanceID, const TCHAR* functionName, std::vector<float> floats, std::vector<int> ints, std::vector<bool> bools, std::vector<const TCHAR*> strings, std::vector<LONGLONG> objects) const override;
	virtual bool CallCSFunctionBool(int appDomainID, __int64 instanceID, const TCHAR* functionName, std::vector<float> floats, std::vector<int> ints, std::vector<bool> bools, std::vector<const TCHAR*> strings, std::vector<LONGLONG> objects) const override;
	virtual const TCHAR* CallCSFunctionString(int appDomainID, __int64 instanceID, const TCHAR* functionName, std::vector<float> floats, std::vector<int> ints, std::vector<bool> bools, std::vector<const TCHAR*> strings, std::vector<LONGLONG> objects) const override;
	virtual UObject* CallCSFunctionObject(int appDomainID, __int64 instanceID, const TCHAR* functionName, std::vector<float> floats, std::vector<int> ints, std::vector<bool> bools, std::vector<const TCHAR*> strings, std::vector<LONGLONG> objects) const override;
	virtual void CallCSFunctionVoid(int appDomainID, __int64 instanceID, const TCHAR* functionName, std::vector<float> floats, std::vector<int> ints, std::vector<bool> bools, std::vector<const TCHAR*> strings, std::vector<LONGLONG> objects) const override;

	virtual const TCHAR* GetAssemblyInfo(int appDomainID) const override;
public:
	ClrHost() : _hostControl(nullptr) {}
	void CreateSafeArrayBool(std::vector<bool>* bools, SAFEARRAY** boolsArray) const;
	void CreateSafeArrayString(std::vector<const TCHAR*>* strings, SAFEARRAY** stringsArray) const;
private:
	class ClrHostControl* _hostControl;
	ICLRRuntimeHostPtr _runtimeHost;

	struct ClassWrapperInfo
	{
		void** functionPointers;
		int numFunctions;
	};
	std::map<tstring, ClassWrapperInfo> _classWrappers;
	tstring _engineAppDomainAppBase;
	tstring _gameScriptsAssemblyName;
};

} // namespace Klawr
