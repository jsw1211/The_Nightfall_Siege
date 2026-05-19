#pragma once

constexpr short PORT = 3500;
constexpr int WORLD_WIDTH = 2000;
constexpr int WORLD_HEIGHT = 2000;
constexpr int MAX_PLAYERS = 10000;
constexpr int NPC_ID_START = MAX_PLAYERS;
constexpr int MAX_NPCS = 200000;
constexpr int MAX_NAME_LEN = 20;

enum PACKET_TYPE {
	C2S_LOGIN,
	C2S_MOVE,
	S2C_LOGIN_RESULT,
	S2C_AVATAR_INFO,
	S2C_ADD_PLAYER,
	S2C_REMOVE_PLAYER,
	S2C_MOVE_PLAYER
};

enum DIRECTION {
	DIR_UP,
	DIR_DOWN,
	DIR_LEFT,
	DIR_RIGHT
};

#pragma pack(push, 1)
struct C2S_Login {
	unsigned char	size;
	PACKET_TYPE		type;
	char			m_username[MAX_NAME_LEN];
};

struct C2S_Move {
	unsigned char	size;
	PACKET_TYPE		type;
	short			x;
	short			y;	
	int 			move_time;
};

struct S2C_LoginResult {
	unsigned char	size;
	PACKET_TYPE		type;
	bool			success;
	char			message[50];
};

struct S2C_AvatarInfo {
	unsigned char	size;
	PACKET_TYPE		type;
	int 			playerId;
	short			x;
	short			y;
};

struct S2C_AddPlayer {
	unsigned char	size;
	PACKET_TYPE		type;
	int 			playerId;
	char			m_username[MAX_NAME_LEN];
	short			x;
	short			y;
};

struct S2C_RemovePlayer {
	unsigned char	size;
	PACKET_TYPE		type;
	int 			playerId;
};

struct S2C_MovePlayer {
	unsigned char	size;
	PACKET_TYPE		type;
	int 			playerId;
	short			x;
	short			y;
	int				move_time;
};
#pragma pack(pop)