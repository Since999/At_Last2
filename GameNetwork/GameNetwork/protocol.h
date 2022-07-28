#pragma once
#include <iostream>
#include <thread>
#include <array>
#include <vector>
#include <fstream>
#include <mutex>
#include <cmath>
#include <algorithm>

#include <WinSock2.h>
#include <WS2tcpip.h>
#include <MSWSock.h>

#include <unordered_set>
#include <queue>
#include <atomic>

#pragma comment(lib, "ws2_32.lib")
#pragma comment (lib, "MSWSock.LIB")

using namespace std;

const short SERVER_PORT = 4000;
const char* const SERVER_IP = "220.94.221.236";
//const char* const SERVER_IP = "127.0.0.1";	// �ӽ÷� �ڱ� IP ����Ű�� ����

const int MAX_PLAYER = 3;							// �÷��̾� ��
const int MAX_ZOMBIE_NPC = 100;				// �ִ� ���� ��
const int MAX_OBJECT = 100;
const int MAX_BUFFER_SIZE = 4096;			// ���� ũ��
const int MAX_CHAT_SIZE = 200;

const int MAX_NAME_SIZE = 20;					// �÷��̾� �̸� ����

const int ROAD_ZOMBIE_NUM = 20;

const int FIRST_CHECK_POINT_ZOMBIE_NUM = 100;
const int TWO_CHECK_POINT_ZOMBIE_NUM = 150;
const int THREE_CHECK_POINT_ZOMBIE_NUM = 200;

const int WORLD_WIDTH = 1100;
const int WORLD_HEIGHT = 420;

/*
------------------------ �߰��Ǿ�� ������ �𸣴� ���� ��� ------------------------

1. �ִϸ��̼� enum class -> �÷��̾� ���� ��, �̵�, ���� ����, ����, �ǰ� �� �� ���ϴ� �ִϸ��̼�
2. ���� ���� ��� -> � �� ��� �������� �������� ��� �ʿ�. Ȯ���ں��� ���� ���, ����
3. ������Ʈ�� ��ġ�ϴ� ������Ʈ��? �ƴϸ� �� ��ü??
4. ������ Ŭ���̾�Ʈ MsgType�� �ٸ��� �ؾ��� �� ����. ���߿� Ȯ���ϰ� ���� �ʿ�

---------------------------------------------------------------------------
*/

enum class IOType : char
{
	SEND,
	RECV,
	ACCEPT,
	NPC_SPAWN,
	NPC_MOVE,
	NPC_ATTACK,
	NPC_DEAD,
	NPC_SEND,
	DOOR_OPEN,
	PLAYER_MOVE
};

enum class ClientState : char
{
	FREE,													// Ŭ���̾�Ʈ�� �ƹ��� ��û�� ���� ���� ����
	ACCEPT,												// Ŭ���̾�Ʈ�� ������ ACCEPT�� ����
	INGAME,												// Ŭ���̾�Ʈ�� INGAME���� �ִ� ����
	DEAD													// Ŭ���̾�Ʈ�� �׾ �ƹ��͵� ���� ���ϴ� ����
};

enum class ZombieState : char
{
	FREE,
	SLEEP,
	SPAWN,
	ACTIVE,
	DEAD
};

enum class MsgType : char							// �������� ������ �޼��� ����
{
	CS_LOGIN_REQUEST,								// Ŭ�� -> ���� �α��� ��û
	CS_BARRICADE_REQUEST,						// Ŭ�� -> ���� �ٸ�����Ʈ ��ġ ��û
	CS_GAME_START_REQUEST,					// Ŭ�� -> ���� ���� ���� ��û
	CS_GAME_START,									// Ŭ�� -> ���� ���� ���� Ȯ�� ����
	CS_PLAYER_SELECT,								// �÷��̾� ĳ���� ���ý�(������ �÷��̾� ����)
	CS_PLAYER_CHAT,									// �÷��̾� ä��(Ŭ�󿡼� ������)
	CS_PLAYER_MOVE,									// �÷��̾� �̵� (Ŭ�� ����)
	CS_PLAYER_ATTACK,								// �÷��̾� ����
	CS_PLAYER_ROTATE,								// �÷��̾� ȸ��
	CS_PLAYER_BUILD,								// �÷��̾� ��ġ
	CS_PLAYER_HIDE,									// �÷��̾� ����
	CS_PLAYER_INTERATION,						// �÷��̾� ��ȣ�ۿ�
	CS_PLAYER_SPECIAL,								// �÷��̾� Ư�� �ɷ� ���
	CS_PLAYER_RELOAD_REQUEST,				// �÷��̾� ������ ��ȣ
	CS_SPECIAL_SKILL_CHANGE,					// ����� ��ų ���� ��û
	CS_SPECIAL_SKILL_REQUEST,					// ����� ��ų ��� ��û
	CS_SERVER_REQUEST,							// ���� ���� ��û
	CS_GM_MAP_CHANGE_ROAD_ONE,			// �� 1�� �̵�									Ű : I
	CS_GM_MAP_CHANGE_ROAD_TWO,			// �� 2�� �̵�									Ű : O
	CS_GM_MAP_CHANGE_ROAD_THREE,		// �� 3���� �̵�								Ű : P
	CS_GM_MAP_CHANGE_BASE_ONE,			// 1�������� �̵�							Ű : J
	CS_GM_MAP_CHANGE_BASE_TWO,			// 2�������� �̵�							Ű : K
	CS_GM_MAP_CHANGE_BASE_THREE,		// 3�������� �̵�							Ű : L
	CS_GM_ZOMBIE_ALL_KILL,						// ������ ���� ��� ����					Ű : N
	CS_GM_PLAYER_HP_UP,							// �÷��̾� HP �ٽ� Ǯ����				Ű : M
	SC_GM_MAP_CHANGE_MAP,					// �������� �� �����ߴٰ� ����
	SC_GM_PLAYER_HP_UP,							// �������� �ذ�� ����
	SC_LOGIN_OK,										// ���� -> Ŭ�� �α��� ���� ��ȣ
	SC_LOGIN_OTHER,									// �ٸ� �÷��̾� �α��ν�
	SC_LOGIN_FAIL,										// �α��� ���н�
	SC_PLAYER_SELECT,								// ��� �÷��̾� ĳ���� ���� �Ϸ�(Ÿ �÷��̾� ĳ���� ����)
	SC_PLAYER_SELECT_FAIL,						// �÷��̼� ���� ����
	SC_BARRICADE_SEND,							// �ٸ����̵� ��ġ �����ֱ�
	SC_GAME_START,									// ���� ����
	SC_GAME_START_FAIL,							// ���� ���� ����(���� Ÿ �÷��̾� �������� ����, �Ǵ� ������ ����)
	SC_PLAYER_CHAT,									// �÷��̾� ä��(�������� ��� Ŭ�󿡰�)
	SC_PLAYER_MOVE,									// �÷��̾� �̵� ����(������ ��� Ŭ��)	
	SC_PLAYER_MOVE_FAIL,							// �÷��̾� �̵� ����
	SC_PLAYER_ROTATE,								// �÷��̾ ȸ��
	SC_PLAYER_ATTACK,								// �÷��̾� ����
	SC_PLAYER_KILL_NUMBER,						// �÷��̾� ���� ���� ��
	SC_PLAYER_RELOAD_REQUEST,				// �÷��̾� ������ ��û
	SC_PLAYER_RELOAD,								// �÷��̾� ������ �Ϸ�
	SC_PLAYER_BUILD,								// �÷��̾� ��ġ
	SC_PLAYER_INTERATION,						// �÷��̾� ��ȣ�ۿ�
	SC_PLAYER_NOT_ENGINEER,					// �÷��̾ �����Ͼ �ƴϱ⿡ ����� �� ���� ��
	SC_PLAYER_SPECIAL,								// �÷��̾� Ư�� �ɷ� ���
	SC_COMMANDER_SPECIAL,						// �÷��̾�(Ŀ�Ǵ�) Ư�� �ɷ� ���
	SC_COMMANDER_SPECIAL_CHECK,			// �÷��̾�(Ŀ�Ǵ�) Ư�� �ɷ� ��� ���� üũ
	SC_ENGINEER_SPECIAL,							// �÷��̾�(�����Ͼ�) Ư�� �ɷ� ���
	SC_ENGINEER_SPECIAL_CHECK,				// �÷��̾�(�����Ͼ�) Ư�� �ɷ� ��� üũ
	SC_ENGINEER_SPECIAL_BUILD_FAIL,		// �÷��̾�(�����Ͼ�) Ư�� �ɷ� ���� ����
	SC_MERCENARY_SPECIAL,						// �÷��̾�(�뺴) Ư�� �ɷ� ���
	SC_MERCENARY_SPECIAL_CHECK,			// �÷��̾�(�뺴) Ư�� �ɷ� ��� üũ
	SC_PLAYER_SPECIAL_NUM_ZERO,			// �÷��̾� Ư�� �ɷ� ��뷮 ���� �̸�
	SC_PLAYER_DEAD,									// �÷��̾� ���
	SC_PLAYER_SEARCH,								// �÷��̾� �ֺ� �繰 ���� Ȯ��
	SC_PLAYER_IDLE,									// �÷��̾� �����ִ� ����
	//SC_PLAYER_SEARCH_FAIL,					// �÷��̾� �ֺ� �繰 Ȯ�� ����
	SC_ZOMBIE_SPAWN,								// ���� ������ ��� ����, ���� ����
	SC_ZOMBIE_MOVE,									// ���� �̵�
	SC_ZOMBIE_ARRIVE,								// ���� ����
	SC_ZOMBIE_REMAIN,								// ���� �� ���� ��������
	SC_ZOMBIE_NUMBER,								// ���� ���� ����
	SC_ZOMBIE_ATTACK,								// ���� ����
	SC_ZOMBIE_MAD,									// ���� ����ȭ
	SC_ZOMBIE_SEARCH,								// ���� �÷��̾ �߰�
	SC_ZOMBIE_VIEWLIST_PUT,						// ���� �丮��Ʈ
	SC_ZOMBIE_VIEWLIST_REMOVE,				// ���� �丮��Ʈ ����
	SC_ZOMBIE_DEAD,									// ���� ���
	SC_ZOMBIE_ALL_KILL,							// ���� ��� �ѹ��� ���
	SC_UPDATE_PLAYER_INFO,						// �÷��̾� ���� ������Ʈ
	SC_UPDATE_ZOMBIE_INFO,						// ���� ���� ������Ʈ
	SC_UPDATE_OBJECT_INFO,						// ������Ʈ ���� ������Ʈ
	SC_UPDATE_STATE,								// ���� ������Ʈ
	SC_WIN_STATE,										// �¸� ���� Ȯ��
	SC_DOOR_OPEN,									// ���� ������ ��
	SC_WAIT,												// ���� �����
	SC_GAME_END,										// ���� Ŭ���� ����
	SC_GAME_ALL_DEAD_END						// ��ΰ� ���� ����
};

enum class PlayerType : char
{
	NONE,													// ���� �÷��̾ ���� ����
	COMMANDER,										// ���ְ�
	ENGINEER,												// �����Ͼ�
	MERCENARY											// �뺴
};

enum class ZombieType : char
{
	NONE,													// ���� �ƴ� ����(�÷��̾� -> ���� ���ϴ� ����?)
	NORMAL,												// �Ϲ� ����
	SOLIDEIR,												// ���� ����
	TANKER,												// ��Ŀ�� ����
	DOG														// ���� ����
};

enum class ObjectType : char						// ��ȹ �غ��鼭 �� �� ���� �ʿ��ҵ���!
{
	NONE,
	TURRET,
	BARRICADE,
	DOOR,
	WALL
};

enum class ObjectState : char						// ������Ʈ ����
{
	NORMAL,												// ������ ������Ʈ
	BIT_BREAK,											// �ణ �μ��� ������Ʈ
	BREAK													// �μ��� ������Ʈ
};

enum class WinGameState : char
{
	NONE,													// ������ ���� ��
	WIN_PLAYER,											// �÷��̾ Ż�⿡ �����ϸ�
	LOSE_PLAYER										// ��� �÷��̾ �װų�, Ż�⿡ �����ϸ�
};

enum class Direction : char
{
	UP,														// ��
	DOWN,													// �Ʒ�
	LEFT,													// ��
	RIGHT,													// ����
	UP_RIGHT,												// �ϵ�
	UP_LEFT,												// �ϼ�
	DOWN_RIGHT,										// ����
	DOWN_LEFT,											// ����
	NONE													// ���� ����
};

enum class MapType : char
{
	NONE,
	SPAWN,													// ���� ����
	FIRST_PATH,											// ù��° ��
	SECOND_PATH,										// �ι�° ��
	FINAL_PATH,											// ����° ��
	CHECK_POINT_ONE,								// ù��° ����
	CHECK_POINT_TWO,								// �ι�° ����
	CHECK_POINT_FINAL,								// ����° ����
	EXIT														// Ż�ⱸ
};

enum class MazeWall : char							// �� ����
{
	WALL = '0',											// ��
	ROAD = '1',											// ��(�̵� ����)
	BARRICADE = '2',									// �ٸ�����Ʈ
	DOOR = '3',											// ��
	DOT = '5'												// �̷� ���� �� �ʿ�
};

enum class DIR : char									// ���� , ����
{
	WIDTH,
	HEIGHT
};

enum class BarricadeType : char
{
	CAR,														// 1���� ����
	TREE,													// 2���� ����
	BARRICADE											// 3���� �����Ͼ� ��ų, �ٸ�����Ʈ
};

enum class ANGLE : char
{
	ZERO = 0,
	FIFTEEN = 1,
	THIRTY = 2,
	FORTY_FIVE = 3,
	SIXTY = 4,
	SEVENTY_FIVE = 5,
	NINETY = 6,
	ONE_HUNDRED_FIVE = 7,
	ONE_HUNDRED_TWENTY = 8,
	ONE_HUNDRED_THIRTY_FIVE =9,
	ONE_HUNDERED_FIFTY = 10,
	ONE_HUNDRED_SIXTY_FIVE = 11,
	ONE_HUNDRED_EIGHTY = 12,
	ONE_HUNDRED_NINETY_FIVE = 13,
	TWO_HUNDRED_TEN = 14,
	TWO_HUNDRED_TWENTY_FIVE = 15,
	TWO_HUNDRED_FORTY = 16,
	TWO_HUNDRED_FIFTY_FIVE = 17,
	TWO_HUNDRED_SEVENTY = 18,
	TWO_HUNDRED_EIGHTY_FIVE = 19,
	THREE_HUNDRED = 20,
	THREE_HUNDRED_FIFTEEN = 21,
	THREE_HUNDRED_THIRD = 22,
	THREE_HUNDRED_FORTY_FIVE = 23
};

struct VectorBox												// ��ü�� �ٿ�� �ڽ� ǥ���ϱ� ���� ����ϴ� vector4 ����ü 
{
	float MaxX;
	float MinX;
	float MaxZ;
	float MinZ;
};

struct iPos {
	short x;
	short z;
	ANGLE angle;
	BarricadeType b_type;
};

#pragma pack(push, 1)

struct cs_login_packet									// Ŭ���̾�Ʈ���� ������ �α��� ��Ŷ ����
{
	unsigned short size;									// ������ ����
	MsgType type;										// �޽��� Ÿ�� LOGIN_REQUEST
};

struct cs_move_packet								// Ŭ���̾�Ʈ���� ������ �����̴� ���� ����
{
	unsigned short size;
	MsgType type;										// �޽��� Ÿ�� PLAYER_MOVE
	float t_x, t_z;											// �÷��̾� ����
	float speed;											// �÷��̾� ���ǵ�
	float x, z;												// �÷��̾� ��ǥ
	float rotation_angle;									// �÷��̾� ȸ��
	bool in_input;											// �÷��̾� �Է��ߴ°�?
};

struct cs_rotate_packet								// Ŭ���̾�Ʈ���� ������ �÷��̾ ȸ���� ���콺 ��ġ�� ����
{
	unsigned short size;
	MsgType type;										// �޽��� Ÿ�� PLAYER_ROTATE
	float mx, mz;											// �÷��̾� ���콺 ��ǥ
};

struct cs_chat_packet									// Ŭ���̾�Ʈ���� ������ ä�� �޽��� ����
{
	unsigned short size;
	MsgType type;										// �޽��� Ÿ�� PLAYER_CHAT
	char message[MAX_CHAT_SIZE];				// �÷��̾� ä�� �޽���
};

struct cs_attack_packet								// Ŭ���̾�Ʈ���� ������ ���� ���� ����
{
	unsigned short size;
	MsgType type;										// �޽��� Ÿ�� PLYAER_ATTACK
	short x, z;												// ������ ��ǥ(���콺 Ŭ�� ��ġ) x,z
	float mx, mz;											// ���� ����
};

struct cs_build_packet									// Ŭ���̾�Ʈ���� ��� ��ġ�� ������ ��ġ�� ������ ����
{
	unsigned short size;
	MsgType type;										// �޽��� Ÿ�� PLAYER_BUILD
	float x, z;												// ��ġ�� ��ġ
	ObjectType object;									// ������Ʈ ����
};

struct cs_interation_packet							// ������Ʈ ��ȣ�ۿ� ��û ��Ŷ
{
	unsigned short size;
	MsgType type;										// �޽��� Ÿ�� PLAYER_INTERATION
	//char id;													// ������Ʈ ���̵�
	//ObjectType object;									// ������Ʈ ����
};

struct cs_select_packet								// Ŭ���̾�Ʈ���� � ĳ���͸� �������� ����
{
	unsigned short size;
	MsgType type;										// �޽��� Ÿ�� PLAYER_SELECT
	PlayerType playertype;								// �� �÷��̾� ����
};

struct cs_request_packet									// Ŭ���̾�Ʈ���� ������ �����ߴ��� Ȯ���ϰ��� �� �� ����
{
	unsigned short size;
	MsgType type;										// �޽��� Ÿ�� CS_GAME_START_REQUEST
};

struct cs_special_req_packet						// Ŭ���̾�Ʈ���� Ŀ�Ǵ��� ���� �츱�� ��û�ϴ� ��Ŷ
{
	unsigned short size;
	MsgType type;										// �޽��� Ÿ�� CS_SPECAIL_SKILL_REQUEST
	int id;														// Ŀ�Ǵ� �츱 ID
};

struct sc_login_ok_packet							// �������� Ŭ���̾�Ʈ���� �α����� �Ǿ��ٰ� ����
{
	unsigned short size;
	MsgType type;										// �޽��� Ÿ�� LOGIN_OK
	char id;													// Ŭ���̾�Ʈ ID ����
	char select_type;										// 000 NONE, 001 COMMANDER, 010 ENGINEER, 100 MER
};

struct sc_login_other_packet
{
	unsigned short size;
	MsgType type;										// �޽��� Ÿ�� LOGIN_OK
	char id;													// Ŭ���̾�Ʈ ID ����
	char select_type;										// 000 NONE, 001 COMMANDER, 010 ENGINEER, 100 MER
};

struct sc_fail_packet						 			// �������� Ŭ���̾�Ʈ���� �α��ο� �����Ͽ��ٰ� ����
{
	unsigned short size;												
	MsgType type;										// �޽��� Ÿ�� LOGIN_FAIL, SELECT_FAIL
	char id;
};

struct sc_player_select_packet						// �������� Ŭ���̾�Ʈ���� ĳ���� ������ �˷���
{
	unsigned short size;
	MsgType type;										// �޽��� Ÿ�� PLAYER_SELECT_COMPLETE
	PlayerType playertype;								// �÷��̾� Ÿ�� �ش� ĳ���� 
	char id;													// �÷��̾� id
	float x, z;												// �÷��̾� ��ġ
	short hp, maxhp;
	char bullet;												// �÷��̾� �Ѿ� ���� ����
	float speed;											// �÷��̾� �̵� �ӵ�
	char select_type;										// 000 NONE, 001 COMMANDER, 010 ENGINEER, 100 MER
};

struct sc_barricade_packet							// �������� �ٸ����̵� ��ġ �����ִ� ��Ŷ
{
	unsigned short size;
	MsgType type;										// �޽��� Ÿ�� BARRICADE_SEND
	char id;
	iPos one_base[42];								// ù��° ���� �̷� ���� ��ġ, ����
	iPos two_base[32];									// �ι�° ���� �̷� ���� ��ġ, ����
	iPos three_base[30];								// ����° ����1 �̷� ���� ��ġ, ����
	iPos three_base2[30];								// ����° ����2 �̷� ���� ��ġ, ����
};

struct sc_game_start_packet						// �������� ������ �����ϸ� �����ִ� ��Ŷ
{
	unsigned short size;
	MsgType type;										// �޽��� Ÿ�� GAME_START
	char id;
	// �Ƹ� ���� ��� ���� �Ұ� ����
};

struct sc_player_idle_packet							// �������� �÷��̾� ������ �ִٰ� �����ִ� ��Ŷ
{
	unsigned short size;
	MsgType type;										// �޽��� Ÿ�� PLAYER_IDLE
	char id;
};

struct sc_player_move_packet						// �������� �÷��̾ �̵��ϸ� �����ִ� ��Ŷ
{
	unsigned short size;
	MsgType type;										// �޽��� Ÿ�� PLAYER_MOVE
	char id;
	float x, z;												// �÷��̾� ��ġ
	float t_x, t_z;											// �÷��̾� ����
	float speed;
	float rotation_angle;									// �÷��̾� ȸ��
	bool in_input;											// �÷��̾� �Է��ߴ°�?
};

struct sc_player_rotate_packet								// �������� �÷��̾ ȸ���� ���콺 ��ġ�� ����
{
	unsigned short size;
	MsgType type;										// �޽��� Ÿ�� PLAYER_ROTATE
	char id;
	float mx, mz;											// �÷��̾� ���콺 ��ǥ
};

struct sc_player_chat_packet						// �������� ��� Ŭ�󿡰� ä�� �޽����� ����
{
	unsigned short size;
	MsgType type;										// �޽��� Ÿ�� PLAYER_CHAT
	char id;
	char s_id;												// ���� ���´���
	char message[MAX_CHAT_SIZE];				// ��� ���̵𿡰� ���� ���̵�
};

struct sc_player_build_packet						// �������� Ŭ���̾�Ʈ���� ��� ������Ʈ�� ��ġ�� ��ġ�ߴ��� �����ϴ� ��Ŷ
{
	unsigned short size;
	MsgType type;										// �޽��� Ÿ�� PLAYER_BUILD
	char id[MAX_PLAYER];
	float x, z;												// ��ġ�� ������Ʈ ��ġ
	ObjectType object;									// ��ġ�� ������Ʈ ����
};

struct sc_player_reload_packet						// �������� ������ ���� �� ������
{
	unsigned short size;
	MsgType type;										// �޽��� Ÿ�� PlayerReload
	char id;
};

struct sc_player_interation_packet					// �������� Ŭ���̾�Ʈ���� �÷��̾� ��ȣ�ۿ� ��Ŷ
{
	unsigned short size;
	MsgType type;										// �޽��� Ÿ�� PLAYER_INTERACTION
	char id[MAX_PLAYER];
	ObjectState objstate;								// ������Ʈ ���� �μ����ִ�, ���� �μ���, ������
};

struct sc_player_infection_packet					// �������� Ŭ���̾�Ʈ���� �������� �����ִ� ��Ŷ
{
	unsigned short size;
	MsgType type;										// �޽��� Ÿ�� PLYAER_INFECTION
	char id;													// �������� ���θ� �˰� �־ ��
	float infection;										// ������
};

struct sc_player_special_packet					// �������� Ŭ���̾�Ʈ���� Ư���ɷ� ��� �ߴ��� �˷��ִ� ��Ŷ
{
	unsigned short size;
	MsgType type;										// �޽��� Ÿ�� PLAYER_SPECIAL
	char id;													// �ش� Ŭ�� �˰������� ��
	bool success;											// �ߵ� �ߴ��� ���ߴ���
};

struct sc_player_co_special_check_packet		// Ŀ�Ǵ� Ư���ɷ� ��� ���� üũ�ϴ� ��Ŷ
{
	unsigned short size;
	MsgType type;										// SPECIAL_CHECK
	char id;													// �������� �� ����?
};

struct sc_commander_special_packet			// �������� Ŀ�Ǵ��� Ư���ɷ� ����ߴٰ� �˷��ִ� ��Ŷ
{
	unsigned short size;
	MsgType type;										// �޽��� Ÿ�� COMMANDER_SPECIAL
	char id;													// ����� ID
	short hp;												// �ش� ID �� ü��
	char bullet;												// �ش� ID �� �Ѿ� ��
};

struct sc_player_en_special_check_packet		// �����Ͼ� Ư���ɷ� ��� ���� üũ�ϴ� ��Ŷ
{
	unsigned short size;
	MsgType type;										// SPECIAL_CHECK
	short x, z;												// ��� �����ų�
};

struct sc_engineer_barrigate_build_packet		// �������� �����Ͼ Ư���ɷ� ����ߴٰ� �˷��ִ� ��Ŷ
{
	unsigned short size;
	MsgType type;										// �޽��� Ÿ�� ENGINEER_SPECIAL
	char id;
	short x, z;												// �ٸ�����Ʈ �߽� ��ġ
	Direction dir;											// ����
};

struct sc_player_attack_packet						// �������� Ŭ���̾�Ʈ���� ���� �����ϰ� �ִ��� �˷��ֱ�
{
	unsigned short size;
	MsgType type;										// �޽��� Ÿ�� PLAYER_ATTACK
	char id;
	float mx, mz;
};

struct sc_player_dead_packet						// �������� Ŭ���̾�Ʈ���� Ŭ���̾�Ʈ�� �׾��ٰ� ������
{
	unsigned short size;
	MsgType type;										// �޽��� Ÿ�� PLAYER_DEAD
	char id;													// ���� Ŭ���̾�Ʈ ID
};

struct sc_player_hp_packet							// �������� Ŭ���̾�Ʈ���� Ŭ���̾�Ʈ HP�� �� ���Ҵ��� ������
{
	unsigned short size;
	MsgType type;										// �޽��� Ÿ�� UPDATE_PLAYER_INFO
	char id;													// �ش� ID�� hp
	short hp;												// Ŭ���̾�Ʈ��  hp��
};

struct sc_player_zombie_klil_packet				// �������� Ŭ�󿡰� �� Ŭ�� ���� ��� �׿��ٰ� �˷��ִ� ��Ŷ
{
	unsigned short size;
	MsgType type;										// �޽��� Ÿ�� ZOMBIE_KILL_NUM
	char id;													// ����?
	short zom_num;										// ���� ���� ����.
};

struct sc_zombie_spawn_packet					// �������� Ŭ���̾�Ʈ���� ������ ������ ���� ��ġ�� �˷��ִ� ��Ŷ
{
	unsigned short size;
	MsgType type;										// �޽��� Ÿ�� ZOMBIE_SPAWN
	MapType map_type;
	ZombieType zomtype;								// ���� ���� ����
	unsigned char id;
	float x, z;												// ���� ���� ��ġ
	ZombieState state;
	char hp;													// ���� hp
	float angle;												// ���� ���ϴ� ����
};

struct sc_zombie_move_packet						// �������� Ŭ���̾�Ʈ���� ������ ��ġ ����
{
	unsigned short size;
	MsgType type;										// �޽��� Ÿ�� ZOMBIE_MOVE
	MapType map_type;								// ���� �����ϴ� �� ��ġ
	unsigned char id;									// ������ ID
	float x, z;												// ���� �̵��� ��ġ
	float t_x, t_z;											// ������ ����
	float speed;											// ������ ���ǵ�
	Direction dir;											// ���� ����
};

struct sc_zombie_arrive_packet
{
	unsigned short size;
	MsgType type;										// �޽��� Ÿ�� ZOMBIE_ARRIVE
	MapType map_type;								// ���� ��ġ
	unsigned char id;									// � ID�� �����ߴ°�
	Direction dir;											// ���� ����
};

struct sc_zombie_attack_packet					// �������� ���� ������ �ϸ� ����
{
	unsigned short size;
	MsgType type;										// �޽��� Ÿ�� ZOMBIE_ATTACK
	unsigned char id;
	MapType m_type;									// ������
};

struct sc_zombie_search_packet					// �������� ���� �÷��̾ �߰��ϸ� ����
{
	unsigned short size;
	MsgType type;										// �޽��� Ÿ�� ZOMBIE_SEARCH
	unsigned char z_id;									// �߰��������� ID
	char p_id;												// �߰ߴ��� �÷��̾��� ID
	MapType m_type;									// ������
};

struct sc_zombie_mad_packet						// �������� Ŭ���̾�Ʈ���� ���� ����ȭ�ϸ� ����
{
	unsigned short size;
	MsgType type;										// �޽��� Ÿ�� ZOMBIE_MAD
	unsigned char id;									// � ���� ����ȭ �ߴ°�?
};

struct sc_zombie_viewlist_packet					// �������� Ŭ���̾�Ʈ���� �丮��Ʈ ���� �� ���
{
	unsigned short size;
	MsgType type;										// ������ ZOMBIE_VIEWLIST_PUT , ���� ZOMBIE_VIEWLIST_REMOVE
	char id;													// � Ŭ���̾�Ʈ�� ��������
	unsigned char z_id;									// �ְ� �� ����
	MapType m_type;									// ��ġ�� �� ��ġ
	float x, z;												// ���� ��ġ
	MsgType animation;								// ���� �ִϸ��̼�
	ZombieType z_type;								// ���� Ÿ��
};

struct sc_zombie_dead_packet						// �������� Ŭ���̾�Ʈ���� ���� �׾��ٰ� ����
{
	unsigned short size;
	MsgType type;										// �޽��� Ÿ�� ZOMBIE_DEAD
	unsigned char id;
	MapType m_type;									// ��� �ʿ��� �׾���
};

struct sc_zombie_num_packet						// �������� ���� ���� ������� ������ ��Ŷ
{
	unsigned short size;
	MsgType type;										// �޽��� Ÿ�� ZOMBIE_NUM
	unsigned char zombie_num;						// ���� ���� ������
};

struct sc_zombie_all_kill_packet					// �������� ���� �� �׾��ٰ� ������ ��Ŷ
{
	unsigned short size;
	MsgType type;										// �޽��� Ÿ�� ZOMBIE_ALL_KILL
	MapType m_type;									// ���� �� Ÿ��
};

struct sc_update_zombie_info_packet			// �������� ���� ������ ������ ����
{
	unsigned short size;
	MsgType type;										// �޽��� Ÿ�� UPDATE_ZOMBIE_INFO
	unsigned char id;									// ������ ���� ������ ���̵�
	char hp;													// ������ ���� hp
	MapType map_type;								// ���� ��ġ
};

struct sc_update_object_info_packet				// �������� Ŭ���̾�Ʈ���� ������Ʈ ���� ����
{
	unsigned short size;
	MsgType type;										// �޽��� Ÿ�� UPDATE_OBJECT_INFO
	char id;													// ������Ʈ ID
	char hp;													// ������Ʈ HP
	ObjectState state;									// ������Ʈ ����
};

struct sc_win_state_packet							// �������� Ŭ���̾�Ʈ���� ���� ��� ���� �˷��ֱ�
{
	unsigned short size;
	MsgType type;										// �޽��� Ÿ�� WIN_STATE
	WinGameState state;								// ���� ���� Ȯ��
};

struct sc_search_packet								// �������� Ŭ���̾�Ʈ�� �� �Ǵ� ������Ʈ ���� �߰����� �� ���� ����
{
	unsigned short size;
	MsgType type;										// �޽��� Ÿ�� SC_PLAYER_SEARCH
	char id;
	ObjectType obj_type;								// �߰��� ������Ʈ Ÿ��
	short x, z;												// �߰��� ������Ʈ�� ��ġ
};

struct sc_door_open_packet
{
	unsigned short size;
	MsgType type;										// �޽��� Ÿ�� SC_DOOR_OPEN
	char id;
	short row, col;											// �� ��ġ
	short size_x, size_z;									// �� ũ��
};

struct sc_wait_packet
{
	unsigned short size;
	MsgType type;
	char id;
};

struct sc_gm_change_map_packet				// �������� GM ������� �� ���� �Ǿ��ٰ� ���ϴ� ��Ŷ
{
	unsigned short size;
	MsgType type;										// GM_MAP_CHANGE_������
	char id;													// � ID �� �̵��߳�?
	short x, z;												// x, y ��ǥ ����
};

struct sc_gm_player_hp_packet						// �������� GM ������� �÷��̾��� HP�� ȸ����Ű�� ��Ŷ
{
	unsigned short size;
	MsgType type;										// GM_PLAYER_HP_UP
	char id;													// � ID�� ����?
	short hp;												// ���� HP
};
#pragma pack(pop)

struct	 Object
{
	ObjectType type;
	int row, col;
	float x, z;
	ObjectState state;
	float size_x, size_z;
	DIR dir;
	int num;
};

struct MapInfo
{
	vector<Object> Door;
};