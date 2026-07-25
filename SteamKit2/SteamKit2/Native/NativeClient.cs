using System;
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.Runtime.CompilerServices;
using SteamKit2;

namespace SteamKit2.Native
{
    internal sealed class NativeClient : IDisposable
    {
        private static readonly ConcurrentDictionary<IntPtr, NativeClient> s_handles = new();
        private static int s_nextId;

        public IntPtr Handle { get; }
        public SteamClient Client { get; }

        private readonly ConcurrentQueue<NativeCallback> m_callbacks = new();
        private readonly int m_id;

        public NativeClient(SteamClient client)
        {
            Client = client;
            m_id = ++s_nextId;
            Handle = new IntPtr(m_id);
            s_handles[Handle] = this;
        }

        public static NativeClient? FromHandle(IntPtr handle)
        {
            return s_handles.TryGetValue(handle, out var native) ? native : null;
        }

        public int CallbackCount => m_callbacks.Count;

        public void ProcessCallbacks(int timeoutMs)
        {
            while (true)
            {
                var cb = Client.GetCallback();
                if (cb == null)
                    break;
                m_callbacks.Enqueue(NativeCallback.FromCallbackMsg(cb));
            }
        }

        public uint GetCallbackType(int index)
        {
            if (TryGetCallback(index, out var cb))
                return (uint)cb.Type;
            return 0;
        }

        public ulong GetCallbackSteamID(int index)
        {
            if (TryGetCallback(index, out var cb))
                return cb.SteamID;
            return 0;
        }

        public ulong GetCallbackJobID(int index)
        {
            if (TryGetCallback(index, out var cb))
                return cb.JobID;
            return 0;
        }

        public int GetCallbackResult(int index)
        {
            if (TryGetCallback(index, out var cb))
                return cb.Result;
            return 0;
        }

        public string GetCallbackString(int index)
        {
            if (TryGetCallback(index, out var cb))
                return cb.StringValue;
            return null;
        }

        private bool TryGetCallback(int index, out NativeCallback cb)
        {
            var list = m_callbacks.ToArray();
            if (index >= 0 && index < list.Length)
            {
                cb = list[index];
                return true;
            }
            cb = default;
            return false;
        }

        public void Dispose()
        {
            Client.Disconnect();
            System.GC.KeepAlive(Client);
        }
    }
}