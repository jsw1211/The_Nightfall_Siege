#include <iostream>
#include <WS2tcpip.h>
#include <array>
#include <MSWSock.h>
#include "protocol.h"

#pragma comment(lib, "MSWSock.lib")
#pragma comment(lib, "WS2_32.lib")
using namespace std;

constexpr int BUF_SIZE = 200;

enum IOType { IO_SEND, IO_RECV, IO_ACCEPT };

class EXP_OVER {
public:
	WSAOVERLAPPED m_over;
	IOType  m_iotype;
	WSABUF	m_wsa;
	char  m_buff[BUF_SIZE];
	EXP_OVER()
	{
		ZeroMemory(&m_over, sizeof(m_over));
		m_wsa.buf = m_buff;
		m_wsa.len = BUF_SIZE;
	}
	EXP_OVER(IOType iot) : m_iotype(iot)
	{
		ZeroMemory(&m_over, sizeof(m_over));
		m_wsa.buf = m_buff;
		m_wsa.len = BUF_SIZE;
	}
};

void error_display(const wchar_t* msg, int err_no)
{
	WCHAR* lpMsgBuf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, err_no,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf, 0, NULL);
	std::wcout << msg;
	std::wcout << L" === 에러 " << lpMsgBuf << std::endl;
	while (true);   // 디버깅 용
	LocalFree(lpMsgBuf);
}

class SESSION {
public:
	SOCKET		m_client;
	int			m_id;
	bool		m_is_connected;
	EXP_OVER	m_recv_over;
	int			m_prev_recv;
	char		m_username[MAX_NAME_LEN];
	short		m_x, m_y;
	SESSION() {
		m_is_connected = false;
		m_id = 999;
		m_client = INVALID_SOCKET;
		m_recv_over.m_iotype = IO_RECV;
		m_x = 0; m_y = 0;
		m_prev_recv = 0;
	}
	~SESSION()
	{
		if (m_is_connected)
			closesocket(m_client);
	}
	void do_recv()
	{
		DWORD recv_flag = 0;
		memset(&m_recv_over.m_over, 0, sizeof(m_recv_over.m_over));
		WSARecv(m_client, &m_recv_over.m_wsa, 1, 0, &recv_flag, &m_recv_over.m_over, nullptr);
	}
	void do_send(int num_bytes, char* mess)
	{
		EXP_OVER* o = new EXP_OVER(IO_SEND);
		o->m_wsa.len = num_bytes;
		memcpy(o->m_buff, mess, num_bytes);
		WSASend(m_client, &o->m_wsa, 1, 0, 0, &o->m_over, nullptr);
	}
	void send_avatar_info()
	{
		S2C_AvatarInfo packet;
		packet.size = sizeof(S2C_AvatarInfo);
		packet.type = S2C_AVATAR_INFO;
		packet.playerId = m_id;
		packet.x = m_x;
		packet.y = m_y;
		do_send(packet.size, reinterpret_cast<char*>(&packet));
	}

	void send_login_success()
	{
		S2C_LoginResult packet;
		packet.size = sizeof(S2C_LoginResult);
		packet.type = S2C_LOGIN_RESULT;
		packet.success = true;
		strncpy_s(packet.message, "Login successful.", sizeof(packet.message));
		do_send(packet.size, reinterpret_cast<char*>(&packet));
	}
	void send_remove_player(int player_id)
	{
		S2C_RemovePlayer packet;
		packet.size = sizeof(S2C_RemovePlayer);
		packet.type = S2C_REMOVE_PLAYER;
		packet.playerId = player_id;
		do_send(packet.size, reinterpret_cast<char*>(&packet));
	}

	void send_move_packet(int mover);
	void send_add_player(int player_id);
	void process_packet(unsigned char* p);
};

std::array<SESSION, MAX_PLAYERS> clients;

void SESSION::send_add_player(int player_id)
{
	S2C_AddPlayer packet;
	packet.size = sizeof(S2C_AddPlayer);
	packet.type = S2C_ADD_PLAYER;
	packet.playerId = player_id;
	SESSION& pl = clients[player_id];
	memcpy(packet.m_username, pl.m_username, sizeof(packet.m_username));
	packet.x = pl.m_x;
	packet.y = pl.m_y;
	do_send(packet.size, reinterpret_cast<char*>(&packet));
}

void SESSION::process_packet(unsigned char* p)
{
	PACKET_TYPE type = *reinterpret_cast<PACKET_TYPE*>(&p[1]);
	switch (type) {
	case C2S_LOGIN: {
		C2S_Login* packet = reinterpret_cast<C2S_Login*>(p);
		strncpy_s(m_username, packet->m_username, MAX_NAME_LEN);
		cout << "Player[" << m_id << "] logged in as " << m_username << endl;
		send_avatar_info();
		for (auto& other : clients) {
			if (false == other.m_is_connected || other.m_id == m_id) continue;
			send_add_player(other.m_id);
			other.send_add_player(m_id);
		}
	}
				  break;
	case C2S_MOVE: {
		C2S_Move* packet = reinterpret_cast<C2S_Move*>(p);
		DIRECTION dir = packet->dir;
		switch (dir) {
		case UP:	m_y = max(0, m_y - 1);					break;
		case DOWN:	m_y = min(WORLD_HEIGHT - 1, m_y + 1);	break;
		case LEFT:	m_x = max(0, m_x - 1);					break;
		case RIGHT: m_x = min(WORLD_WIDTH - 1, m_x + 1);	break;
		}
		cout << "Player[" << m_id << "] moved to (" << m_x << ", " << m_y << ")\n";
		for (auto& cl : clients)
			if (true == cl.m_is_connected)
				cl.send_move_packet(m_id);
	}
				 break;
	default:
		cout << "Unknown packet type received from player[" << m_id << "].\n";
		break;
	}
}

void SESSION::send_move_packet(int mover)
{
	S2C_MovePlayer packet;
	packet.size = sizeof(S2C_MovePlayer);
	packet.type = S2C_MOVE_PLAYER;
	packet.playerId = mover;
	packet.x = clients[mover].m_x;
	packet.y = clients[mover].m_y;
	do_send(packet.size, reinterpret_cast<char*>(&packet));
}

void send_login_fail(SOCKET client, const char* message)
{
	S2C_LoginResult packet;
	packet.size = sizeof(S2C_LoginResult);
	packet.type = S2C_LOGIN_RESULT;
	packet.success = false;
	strncpy_s(packet.message, message, sizeof(packet.message));
	WSABUF wsa_buf;
	wsa_buf.buf = reinterpret_cast<char*>(&packet);
	wsa_buf.len = packet.size;
	WSASend(client, &wsa_buf, 1, 0, 0, nullptr, nullptr);
}

int main()
{
	WSADATA WSAData;
	WSAStartup(MAKEWORD(2, 2), &WSAData);
	SOCKET server = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	SOCKADDR_IN server_addr;
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(PORT);
	server_addr.sin_addr.S_un.S_addr = INADDR_ANY;
	bind(server, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr));
	listen(server, SOMAXCONN);
	HANDLE h_iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	CreateIoCompletionPort((HANDLE)server, h_iocp, -1, 0);

	SOCKET client_socket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);

	EXP_OVER accept_over(IO_ACCEPT);
	AcceptEx(server, client_socket, &accept_over.m_buff, 0,
		sizeof(SOCKADDR_IN) + 16, sizeof(SOCKADDR_IN) + 16,
		NULL, &accept_over.m_over);

	for (int player_index = 0;;) {
		DWORD num_bytes;
		ULONG_PTR key;
		LPOVERLAPPED over;
		BOOL res = GetQueuedCompletionStatus(h_iocp, &num_bytes, &key, &over, INFINITE);

		if (res == FALSE) {
			int err_no = WSAGetLastError();
			if (over != nullptr) {
				int p_idx = static_cast<int>(key);
				if (clients[p_idx].m_is_connected) {
					cout << "Client[" << p_idx << "] disconnected." << endl;
					clients[p_idx].m_is_connected = false;
					closesocket(clients[p_idx].m_client);
					for (auto& cl : clients)
						if (cl.m_is_connected)
							cl.send_remove_player(p_idx);
				}
			}
			else {
				error_display(L"GQCS Error: ", err_no);
			}
			continue;
		}
		EXP_OVER* exp_over = reinterpret_cast<EXP_OVER*>(over);
		int p_idx = static_cast<int>(key);
		if (0 == num_bytes && (exp_over->m_iotype == IO_RECV || exp_over->m_iotype == IO_SEND)) {
			if (clients[p_idx].m_is_connected) {
				cout << "Client[" << p_idx << "] disconnected." << endl;
				clients[p_idx].m_is_connected = false;
				closesocket(clients[p_idx].m_client);
				for (auto& cl : clients)
					if (cl.m_is_connected) cl.send_remove_player(p_idx);
			}
			if (exp_over->m_iotype == IO_SEND) delete exp_over;
			continue;
		}
		switch (exp_over->m_iotype) {
		case IO_ACCEPT:
			cout << "Client connected." << endl;
			player_index = -1;
			for (int i = 0; i < MAX_PLAYERS; ++i)
				if (!clients[i].m_is_connected) {
					player_index = i;
					break;
				}
			if (-1 == player_index) {
				cout << "No more player can be accepted." << endl;
				send_login_fail(client_socket, "Server is full.");
				closesocket(client_socket);
			}
			else {
				CreateIoCompletionPort((HANDLE)client_socket, h_iocp, player_index, 0);
				clients[player_index].m_is_connected = true;
				clients[player_index].m_client = client_socket;
				clients[player_index].m_x = 0;
				clients[player_index].m_y = 0;
				clients[player_index].m_id = player_index;
				clients[player_index].send_login_success();
				clients[player_index].m_prev_recv = 0;

				for (auto& other : clients) {
					if (false == other.m_is_connected) continue;
					if (other.m_id == player_index) continue;
					other.send_add_player(player_index);
					clients[player_index].send_add_player(other.m_id);
				}

				clients[player_index].do_recv();
			}
			client_socket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
			AcceptEx(server, client_socket, &accept_over.m_buff, 0,
				sizeof(SOCKADDR_IN) + 16, sizeof(SOCKADDR_IN) + 16,
				NULL, &accept_over.m_over);
			break;
		case IO_RECV:
		{
			int player_index = static_cast<int>(key);
			cout << "Client[" << player_index << "] sent a message." << endl;
			SESSION& cl = clients[player_index];
			unsigned char* p = reinterpret_cast<unsigned char*>(exp_over->m_buff);
			int data_size = num_bytes + cl.m_prev_recv;
			while (data_size > 0) {
				int packet_size = p[0];
				if (packet_size > data_size) break;
				cl.process_packet(p);
				p += packet_size;
				data_size -= packet_size;
			}
			if (data_size > 0) {
				memmove(cl.m_recv_over.m_buff, p, data_size);
				cl.m_prev_recv = data_size;
			}
			cl.do_recv();
		}
		break;
		case IO_SEND: {
			cout << "Message sent. to client[" << key << "]\n";
			EXP_OVER* o = reinterpret_cast<EXP_OVER*>(over);
			delete o;
		}
					break;
		default:
			cout << "Unknown IO type." << endl;
			exit(-1);
			break;
		}
	}
	closesocket(server);
	WSACleanup();
}
