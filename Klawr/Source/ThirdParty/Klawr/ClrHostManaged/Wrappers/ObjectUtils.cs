﻿//
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

using Klawr.ClrHost.Managed.SafeHandles;
using System;

namespace Klawr.ClrHost.Managed{
    internal class ObjectUtils{
        private static ObjectUtilsProxy _proxy;

        internal ObjectUtils(ref ObjectUtilsProxy proxy){
            _proxy = proxy;
        }

        public static UObjectHandle GetClassByName(string nativeClassName){
            return _proxy.GetClassByName(nativeClassName);
        }

        public static string GetClassName(UObjectHandle nativeClass){
            return _proxy.GetClassName(nativeClass);
        }

        public static bool IsClassChildOf(UObjectHandle derivedClass, UObjectHandle baseClass){
            return _proxy.IsClassChildOf(derivedClass, baseClass);
        }

        /// <summary>
        /// Release a reference to a native UObject instance.
        /// </summary>
        /// <param name="handle">Pointer to a native UObject instance.</param>
        public static void ReleaseObject(IntPtr nativeObject){
            _proxy.RemoveObjectRef?.Invoke(nativeObject);
        }
    }
}