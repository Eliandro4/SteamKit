using System;
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Text;
using SteamKit2;
using SteamKit2.Internal;

namespace SteamKit2.Native
{
    public static class NativeExports
    {
        private static readonly ConditionalWeakTable<SteamClient, NativeClient> s_clientMap = new();

        [UnmanagedCallersOnly(EntryPoint = "SK2_CreateClient", CallConvs = new[] { typeof(CallConvCdecl) })]
        public static IntPtr CreateClient()
        {
            try
            {
                var client = new SteamClient();
                var native = new NativeClient(client);
                s_clientMap.Add(client, native);
                return native.Handle;
            }
            catch
            {
                return IntPtr.Zero;
            }
        }

        [UnmanagedCallersOnly(EntryPoint = "SK2_FreeClient", CallConvs = new[] { typeof(CallConvCdecl) })]
        public static void FreeClient(IntPtr handle)
        {
            if (handle == IntPtr.Zero)
                return;

            if (s_clientMap.TryGetValue(PointerToClient(handle), out var native))
            {
                s_clientMap.Remove(PointerToClient(handle));
                native.Dispose();
            }
        }

        [UnmanagedCallersOnly(EntryPoint = "SK2_Connect", CallConvs = new[] { typeof(CallConvCdecl) })]
        public static void Connect(IntPtr handle)
        {
            var client = PointerToClient(handle);
            if (client != null)
            {
                client.Connect();
            }
        }

        [UnmanagedCallersOnly(EntryPoint = "SK2_Disconnect", CallConvs = new[] { typeof(CallConvCdecl) })]
        public static void Disconnect(IntPtr handle)
        {
            var client = PointerToClient(handle);
            if (client != null)
            {
                client.Disconnect();
            }
        }

        [UnmanagedCallersOnly(EntryPoint = "SK2_IsConnected", CallConvs = new[] { typeof(CallConvCdecl) })]
        public static int IsConnected(IntPtr handle)
        {
            return PointerToClient(handle)?.IsConnected == true ? 1 : 0;
        }

        [UnmanagedCallersOnly(EntryPoint = "SK2_FreeString", CallConvs = new[] { typeof(CallConvCdecl) })]
        public static void FreeString(IntPtr str)
        {
            if (str != IntPtr.Zero)
            {
                Marshal.FreeCoTaskMem(str);
            }
        }

        [UnmanagedCallersOnly(EntryPoint = "SK2_Tick", CallConvs = new[] { typeof(CallConvCdecl) })]
        public static int Tick(IntPtr handle, int timeoutMs)
        {
            var native = TryGetNative(handle);
            if (native == null)
                return 0;

            native.ProcessCallbacks(timeoutMs);
            return native.CallbackCount;
        }

        [UnmanagedCallersOnly(EntryPoint = "SK2_TickNoWait", CallConvs = new[] { typeof(CallConvCdecl) })]
        public static int TickNoWait(IntPtr handle)
        {
            var native = TryGetNative(handle);
            if (native == null)
                return 0;

            native.ProcessCallbacks(0);
            return native.CallbackCount;
        }

        [UnmanagedCallersOnly(EntryPoint = "SK2_GetCallbackCount", CallConvs = new[] { typeof(CallConvCdecl) })]
        public static int GetCallbackCount(IntPtr handle)
        {
            return TryGetNative(handle)?.CallbackCount ?? 0;
        }

        [UnmanagedCallersOnly(EntryPoint = "SK2_GetCallbackType", CallConvs = new[] { typeof(CallConvCdecl) })]
        public static uint GetCallbackType(IntPtr handle, int index)
        {
            return TryGetNative(handle)?.GetCallbackType(index) ?? 0;
        }

        [UnmanagedCallersOnly(EntryPoint = "SK2_GetCallbackSteamID", CallConvs = new[] { typeof(CallConvCdecl) })]
        public static ulong GetCallbackSteamID(IntPtr handle, int index)
        {
            return TryGetNative(handle)?.GetCallbackSteamID(index) ?? 0;
        }

        [UnmanagedCallersOnly(EntryPoint = "SK2_GetCallbackJobID", CallConvs = new[] { typeof(CallConvCdecl) })]
        public static ulong GetCallbackJobID(IntPtr handle, int index)
        {
            return TryGetNative(handle)?.GetCallbackJobID(index) ?? 0;
        }

        [UnmanagedCallersOnly(EntryPoint = "SK2_GetCallbackResult", CallConvs = new[] { typeof(CallConvCdecl) })]
        public static int GetCallbackResult(IntPtr handle, int index)
        {
            return TryGetNative(handle)?.GetCallbackResult(index) ?? 0;
        }

        [UnmanagedCallersOnly(EntryPoint = "SK2_GetCallbackString", CallConvs = new[] { typeof(CallConvCdecl) })]
        public static IntPtr GetCallbackString(IntPtr handle, int index)
        {
            var str = TryGetNative(handle)?.GetCallbackString(index);
            if (str == null)
                return IntPtr.Zero;

            var bytes = Encoding.UTF8.GetBytes(str + '\0');
            var ptr = Marshal.AllocCoTaskMem(bytes.Length);
            Marshal.Copy(bytes, 0, ptr, bytes.Length);
            return ptr;
        }

        [UnmanagedCallersOnly(EntryPoint = "SK2_LogOn", CallConvs = new[] { typeof(CallConvCdecl) })]
        public static void LogOn(IntPtr handle, IntPtr accountName, IntPtr password)
        {
            var client = PointerToClient(handle);
            if (client == null)
                return;

            var name = Marshal.PtrToStringUTF8(accountName);
            var pass = Marshal.PtrToStringUTF8(password);

            var details = new SteamUser.LogOnDetails
            {
                Username = name,
                Password = pass,
            };

            client.GetHandler<SteamUser>()?.LogOn(details);
        }

        [UnmanagedCallersOnly(EntryPoint = "SK2_LogOnAnon", CallConvs = new[] { typeof(CallConvCdecl) })]
        public static void LogOnAnon(IntPtr handle)
        {
            var client = PointerToClient(handle);
            client.GetHandler<SteamUser>()?.LogOnAnonymous();
        }

        [UnmanagedCallersOnly(EntryPoint = "SK2_LogOff", CallConvs = new[] { typeof(CallConvCdecl) })]
        public static void LogOff(IntPtr handle)
        {
            var client = PointerToClient(handle);
            client.GetHandler<SteamUser>()?.LogOff();
        }

        [UnmanagedCallersOnly(EntryPoint = "SK2_SetPersonaState", CallConvs = new[] { typeof(CallConvCdecl) })]
        public static void SetPersonaState(IntPtr handle, uint state)
        {
            var client = PointerToClient(handle);
            if (client == null)
                return;

            client.GetHandler<SteamFriends>()?.SetPersonaState((EPersonaState)state);
        }

        [UnmanagedCallersOnly(EntryPoint = "SK2_SetPersonaName", CallConvs = new[] { typeof(CallConvCdecl) })]
        public static void SetPersonaName(IntPtr handle, IntPtr name)
        {
            var client = PointerToClient(handle);
            if (client == null)
                return;

            var str = Marshal.PtrToStringUTF8(name);
            client.GetHandler<SteamFriends>()?.SetPersonaName(str);
        }

        [UnmanagedCallersOnly(EntryPoint = "SK2_GetPersonaName", CallConvs = new[] { typeof(CallConvCdecl) })]
        public static IntPtr GetPersonaName(IntPtr handle)
        {
            var client = PointerToClient(handle);
            var name = client.GetHandler<SteamFriends>()?.GetPersonaName();
            if (name == null)
                return IntPtr.Zero;

            var bytes = Encoding.UTF8.GetBytes(name + '\0');
            var ptr = Marshal.AllocCoTaskMem(bytes.Length);
            Marshal.Copy(bytes, 0, ptr, bytes.Length);
            return ptr;
        }

        [UnmanagedCallersOnly(EntryPoint = "SK2_GetFriendCount", CallConvs = new[] { typeof(CallConvCdecl) })]
        public static int GetFriendCount(IntPtr handle)
        {
            return PointerToClient(handle)?.GetHandler<SteamFriends>()?.GetFriendCount() ?? 0;
        }

        [UnmanagedCallersOnly(EntryPoint = "SK2_GetFriendByIndex", CallConvs = new[] { typeof(CallConvCdecl) })]
        public static ulong GetFriendByIndex(IntPtr handle, int index)
        {
            var client = PointerToClient(handle);
            var friend = client.GetHandler<SteamFriends>()?.GetFriendByIndex(index);
            return friend?.ConvertToUInt64() ?? 0;
        }

        [UnmanagedCallersOnly(EntryPoint = "SK2_GetFriendPersonaName", CallConvs = new[] { typeof(CallConvCdecl) })]
        public static IntPtr GetFriendPersonaName(IntPtr handle, ulong steamId)
        {
            var client = PointerToClient(handle);
            var name = client.GetHandler<SteamFriends>()?.GetFriendPersonaName(new SteamID(steamId));
            if (name == null)
                return IntPtr.Zero;

            var bytes = Encoding.UTF8.GetBytes(name + '\0');
            var ptr = Marshal.AllocCoTaskMem(bytes.Length);
            Marshal.Copy(bytes, 0, ptr, bytes.Length);
            return ptr;
        }

        [UnmanagedCallersOnly(EntryPoint = "SK2_GetFriendPersonaState", CallConvs = new[] { typeof(CallConvCdecl) })]
        public static uint GetFriendPersonaState(IntPtr handle, ulong steamId)
        {
            var client = PointerToClient(handle);
            return (uint)(client.GetHandler<SteamFriends>()?.GetFriendPersonaState(new SteamID(steamId)) ?? 0);
        }

        [UnmanagedCallersOnly(EntryPoint = "SK2_RemoveFriend", CallConvs = new[] { typeof(CallConvCdecl) })]
        public static void RemoveFriend(IntPtr handle, ulong steamId)
        {
            var client = PointerToClient(handle);
            client.GetHandler<SteamFriends>()?.RemoveFriend(new SteamID(steamId));
        }

        [UnmanagedCallersOnly(EntryPoint = "SK2_GetFriendRelationship", CallConvs = new[] { typeof(CallConvCdecl) })]
        public static uint GetFriendRelationship(IntPtr handle, ulong steamId)
        {
            var client = PointerToClient(handle);
            return (uint)(client.GetHandler<SteamFriends>()?.GetFriendRelationship(new SteamID(steamId)) ?? 0);
        }

        [UnmanagedCallersOnly(EntryPoint = "SK2_AddFriend", CallConvs = new[] { typeof(CallConvCdecl) })]
        public static void AddFriend(IntPtr handle, IntPtr accountNameOrEmail)
        {
            var client = PointerToClient(handle);
            if (client == null)
                return;

            var str = Marshal.PtrToStringUTF8(accountNameOrEmail);
            client.GetHandler<SteamFriends>()?.AddFriend(str);
        }

        private static SteamClient? PointerToClient(IntPtr handle)
        {
            if (handle == IntPtr.Zero)
                return null;

            var native = NativeClient.FromHandle(handle);
            if (native == null)
                return null;

            return native.Client;
        }

        private static NativeClient? TryGetNative(IntPtr handle)
        {
            if (handle == IntPtr.Zero)
                return null;
            return NativeClient.FromHandle(handle);
        }
    }
}