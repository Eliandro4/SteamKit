using ProtoBuf;
using SteamKit2;
using SteamKit2.Internal;

namespace SteamKit2.Native
{
    internal enum SK2_CallbackType : uint
    {
        Unknown = 0,
        Connected = 1,
        Disconnected = 2,
        LoggedOn = 3,
        LoggedOff = 4,
        AccountInfo = 5,
        PersonaState = 6,
        FriendsList = 7,
        FriendAdded = 8,
        FriendMsg = 9,
        WalletInfo = 10,
        SessionToken = 11
    }

    internal readonly struct NativeCallback
    {
        public SK2_CallbackType Type { get; }
        public ulong SteamID { get; }
        public ulong JobID { get; }
        public int Result { get; }
        public string StringValue { get; }

        public NativeCallback(SK2_CallbackType type, ulong steamId, ulong jobId, int result, string? str)
        {
            Type = type;
            SteamID = steamId;
            JobID = jobId;
            Result = result;
            StringValue = str;
        }

        public static NativeCallback FromCallbackMsg(CallbackMsg msg)
        {
            if (msg is SteamClient.ConnectedCallback)
                return new(SK2_CallbackType.Connected, 0, 0, 0, null);
            if (msg is SteamClient.DisconnectedCallback dc)
                return new(SK2_CallbackType.Disconnected, 0, 0, 0, dc.UserInitiated ? "user" : "remote");
            if (msg is SteamUser.LoggedOnCallback loggedOn)
                return new(SK2_CallbackType.LoggedOn, loggedOn.ClientSteamID?.ConvertToUInt64() ?? 0, loggedOn.JobID.BoxID, (int)loggedOn.Result, null);
            if (msg is SteamUser.LoggedOffCallback loggedOff)
                return new(SK2_CallbackType.LoggedOff, 0, loggedOff.JobID.BoxID, (int)loggedOff.Result, null);
            if (msg is SteamUser.AccountInfoCallback accountInfo)
                return new(SK2_CallbackType.AccountInfo, 0, 0, 0, accountInfo.PersonaName);
            if (msg is SteamFriends.PersonaStateCallback persona)
                return new(SK2_CallbackType.PersonaState, persona.FriendID.ConvertToUInt64(), 0, 0, persona.Name);
            if (msg is SteamFriends.FriendsListCallback)
                return new(SK2_CallbackType.FriendsList, 0, 0, 0, null);
            if (msg is SteamFriends.FriendMsgCallback friendMsg)
                return new(SK2_CallbackType.FriendMsg, friendMsg.Sender.ConvertToUInt64(), 0, 0, friendMsg.Message);
            if (msg is SteamFriends.FriendAddedCallback friendAdded)
                return new(SK2_CallbackType.FriendAdded, friendAdded.SteamID.ConvertToUInt64(), 0, (int)friendAdded.Result, null);
            return new(SK2_CallbackType.Unknown, 0, 0, 0, msg.GetType().Name);
        }
    }
}