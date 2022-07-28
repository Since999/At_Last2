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
//const char* const SERVER_IP = "127.0.0.1";	// 임시로 자기 IP 가르키고 있음

const int MAX_PLAYER = 3;							// 플레이어 수
const int MAX_ZOMBIE_NPC = 100;				// 최대 좀비 수
const int MAX_OBJECT = 100;
const int MAX_BUFFER_SIZE = 4096;			// 버퍼 크기
const int MAX_CHAT_SIZE = 200;

const int MAX_NAME_SIZE = 20;					// 플레이어 이름 길이

const int ROAD_ZOMBIE_NUM = 20;

const int FIRST_CHECK_POINT_ZOMBIE_NUM = 100;
const int TWO_CHECK_POINT_ZOMBIE_NUM = 150;
const int THREE_CHECK_POINT_ZOMBIE_NUM = 200;

const int WORLD_WIDTH = 1100;
const int WORLD_HEIGHT = 420;

/*
------------------------ 추가되어야 할지도 모르는 사항 기록 ------------------------

1. 애니메이션 enum class -> 플레이어 선택 시, 이동, 상태 변경, 공격, 피격 등 때 변하는 애니메이션
2. 파일 전송 방법 -> 어떨 때 어떤걸 어떤방법으로 보낼건지 고민 필요. 확장자부터 전송 방식, 순서
3. 오브젝트는 설치하는 오브젝트만? 아니면 맵 전체??
4. 서버와 클라이언트 MsgType을 다르게 해야할 수 있음. 나중에 확인하고 변경 필요

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
	FREE,													// 클라이언트가 아무런 요청도 받지 않은 상태
	ACCEPT,												// 클라이언트와 서버가 ACCEPT된 상태
	INGAME,												// 클라이언트가 INGAME내에 있는 상태
	DEAD													// 클라이언트가 죽어서 아무것도 하지 못하는 상태
};

enum class ZombieState : char
{
	FREE,
	SLEEP,
	SPAWN,
	ACTIVE,
	DEAD
};

enum class MsgType : char							// 서버에서 보내는 메세지 형식
{
	CS_LOGIN_REQUEST,								// 클라 -> 서버 로그인 요청
	CS_BARRICADE_REQUEST,						// 클라 -> 서버 바리게이트 위치 요청
	CS_GAME_START_REQUEST,					// 클라 -> 서버 게임 시작 요청
	CS_GAME_START,									// 클라 -> 서버 게임 시작 확인 응답
	CS_PLAYER_SELECT,								// 플레이어 캐릭터 선택시(선택한 플레이어 전송)
	CS_PLAYER_CHAT,									// 플레이어 채팅(클라에서 서버로)
	CS_PLAYER_MOVE,									// 플레이어 이동 (클라서 서버)
	CS_PLAYER_ATTACK,								// 플레이어 공격
	CS_PLAYER_ROTATE,								// 플레이어 회전
	CS_PLAYER_BUILD,								// 플레이어 설치
	CS_PLAYER_HIDE,									// 플레이어 숨기
	CS_PLAYER_INTERATION,						// 플레이어 상호작용
	CS_PLAYER_SPECIAL,								// 플레이어 특수 능력 사용
	CS_PLAYER_RELOAD_REQUEST,				// 플레이어 재장전 신호
	CS_SPECIAL_SKILL_CHANGE,					// 스페셜 스킬 변경 요청
	CS_SPECIAL_SKILL_REQUEST,					// 스페셜 스킬 사용 요청
	CS_SERVER_REQUEST,							// 서버 응답 요청
	CS_GM_MAP_CHANGE_ROAD_ONE,			// 길 1로 이동									키 : I
	CS_GM_MAP_CHANGE_ROAD_TWO,			// 길 2로 이동									키 : O
	CS_GM_MAP_CHANGE_ROAD_THREE,		// 길 3으로 이동								키 : P
	CS_GM_MAP_CHANGE_BASE_ONE,			// 1거점으로 이동							키 : J
	CS_GM_MAP_CHANGE_BASE_TWO,			// 2거점으로 이동							키 : K
	CS_GM_MAP_CHANGE_BASE_THREE,		// 3거점으로 이동							키 : L
	CS_GM_ZOMBIE_ALL_KILL,						// 누르면 좀비 모두 죽음					키 : N
	CS_GM_PLAYER_HP_UP,							// 플레이어 HP 다시 풀로참				키 : M
	SC_GM_MAP_CHANGE_MAP,					// 서버에서 맵 변경했다고 전달
	SC_GM_PLAYER_HP_UP,							// 서버에서 해결시 전달
	SC_LOGIN_OK,										// 서버 -> 클라 로그인 승인 신호
	SC_LOGIN_OTHER,									// 다른 플레이어 로그인시
	SC_LOGIN_FAIL,										// 로그인 실패시
	SC_PLAYER_SELECT,								// 모든 플레이어 캐릭터 선택 완료(타 플레이어 캐릭터 여부)
	SC_PLAYER_SELECT_FAIL,						// 플레이서 선택 실패
	SC_BARRICADE_SEND,							// 바리케이드 위치 보내주기
	SC_GAME_START,									// 게임 시작
	SC_GAME_START_FAIL,							// 게임 시작 실패(아직 타 플레이어 접속하지 않음, 또는 모종의 이유)
	SC_PLAYER_CHAT,									// 플레이어 채팅(서버에서 모든 클라에게)
	SC_PLAYER_MOVE,									// 플레이어 이동 전송(서버서 모든 클라)	
	SC_PLAYER_MOVE_FAIL,							// 플레이어 이동 실패
	SC_PLAYER_ROTATE,								// 플레이어가 회전
	SC_PLAYER_ATTACK,								// 플레이어 공격
	SC_PLAYER_KILL_NUMBER,						// 플레이어 좀비 죽인 수
	SC_PLAYER_RELOAD_REQUEST,				// 플레이어 재장전 요청
	SC_PLAYER_RELOAD,								// 플레이어 재장전 완료
	SC_PLAYER_BUILD,								// 플레이어 설치
	SC_PLAYER_INTERATION,						// 플레이어 상호작용
	SC_PLAYER_NOT_ENGINEER,					// 플레이어가 엔지니어가 아니기에 사용할 수 없을 때
	SC_PLAYER_SPECIAL,								// 플레이어 특수 능력 사용
	SC_COMMANDER_SPECIAL,						// 플레이어(커맨더) 특수 능력 사용
	SC_COMMANDER_SPECIAL_CHECK,			// 플레이어(커맨더) 특수 능력 사용 여부 체크
	SC_ENGINEER_SPECIAL,							// 플레이어(엔지니어) 특수 능력 사용
	SC_ENGINEER_SPECIAL_CHECK,				// 플레이어(엔지니어) 특수 능력 사용 체크
	SC_ENGINEER_SPECIAL_BUILD_FAIL,		// 플레이어(엔지니어) 특수 능력 빌드 실패
	SC_MERCENARY_SPECIAL,						// 플레이어(용병) 특수 능력 사용
	SC_MERCENARY_SPECIAL_CHECK,			// 플레이어(용병) 특수 능력 사용 체크
	SC_PLAYER_SPECIAL_NUM_ZERO,			// 플레이어 특수 능력 사용량 제로 이면
	SC_PLAYER_DEAD,									// 플레이어 사망
	SC_PLAYER_SEARCH,								// 플레이어 주변 사물 접근 확인
	SC_PLAYER_IDLE,									// 플레이어 멈춰있는 상태
	//SC_PLAYER_SEARCH_FAIL,					// 플레이어 주변 사물 확인 실패
	SC_ZOMBIE_SPAWN,								// 좀비 스폰시 장소 지정, 좀비 지정
	SC_ZOMBIE_MOVE,									// 좀비 이동
	SC_ZOMBIE_ARRIVE,								// 좀비 도착
	SC_ZOMBIE_REMAIN,								// 좀비 문 열기 남아있음
	SC_ZOMBIE_NUMBER,								// 좀비 남은 숫자
	SC_ZOMBIE_ATTACK,								// 좀비 공격
	SC_ZOMBIE_MAD,									// 좀비 광폭화
	SC_ZOMBIE_SEARCH,								// 좀비가 플레이어를 발견
	SC_ZOMBIE_VIEWLIST_PUT,						// 좀비 뷰리스트
	SC_ZOMBIE_VIEWLIST_REMOVE,				// 좀비 뷰리스트 제거
	SC_ZOMBIE_DEAD,									// 좀비 사망
	SC_ZOMBIE_ALL_KILL,							// 좀비 모두 한번에 사망
	SC_UPDATE_PLAYER_INFO,						// 플레이어 정보 업데이트
	SC_UPDATE_ZOMBIE_INFO,						// 좀비 정보 업데이트
	SC_UPDATE_OBJECT_INFO,						// 오브젝트 정보 업데이트
	SC_UPDATE_STATE,								// 상태 업데이트
	SC_WIN_STATE,										// 승리 상태 확인
	SC_DOOR_OPEN,									// 문이 열렸을 때
	SC_WAIT,												// 서버 응답시
	SC_GAME_END,										// 게임 클리어 엔딩
	SC_GAME_ALL_DEAD_END						// 모두가 죽은 엔딩
};

enum class PlayerType : char
{
	NONE,													// 아직 플레이어를 고르지 않음
	COMMANDER,										// 지휘관
	ENGINEER,												// 엔지니어
	MERCENARY											// 용병
};

enum class ZombieType : char
{
	NONE,													// 좀비가 아닌 상태(플레이어 -> 좀비 변하는 상태?)
	NORMAL,												// 일반 좀비
	SOLIDEIR,												// 군인 좀비
	TANKER,												// 탱커형 좀비
	DOG														// 사족 좀비
};

enum class ObjectType : char						// 기획 해보면서 좀 더 상의 필요할듯함!
{
	NONE,
	TURRET,
	BARRICADE,
	DOOR,
	WALL
};

enum class ObjectState : char						// 오브젝트 상태
{
	NORMAL,												// 멀쩡한 오브젝트
	BIT_BREAK,											// 약간 부서진 오브젝트
	BREAK													// 부서진 오브젝트
};

enum class WinGameState : char
{
	NONE,													// 게임이 진행 중
	WIN_PLAYER,											// 플레이어가 탈출에 성공하면
	LOSE_PLAYER										// 모든 플레이어가 죽거나, 탈출에 실패하면
};

enum class Direction : char
{
	UP,														// 위
	DOWN,													// 아래
	LEFT,													// 왼
	RIGHT,													// 오른
	UP_RIGHT,												// 북동
	UP_LEFT,												// 북서
	DOWN_RIGHT,										// 남동
	DOWN_LEFT,											// 남서
	NONE													// 정지 상태
};

enum class MapType : char
{
	NONE,
	SPAWN,													// 스폰 지역
	FIRST_PATH,											// 첫번째 길
	SECOND_PATH,										// 두번째 길
	FINAL_PATH,											// 세번째 길
	CHECK_POINT_ONE,								// 첫번째 거점
	CHECK_POINT_TWO,								// 두번째 거점
	CHECK_POINT_FINAL,								// 세번째 거점
	EXIT														// 탈출구
};

enum class MazeWall : char							// 벽 상태
{
	WALL = '0',											// 벽
	ROAD = '1',											// 길(이동 가능)
	BARRICADE = '2',									// 바리게이트
	DOOR = '3',											// 문
	DOT = '5'												// 미로 만들 때 필요
};

enum class DIR : char									// 가로 , 세로
{
	WIDTH,
	HEIGHT
};

enum class BarricadeType : char
{
	CAR,														// 1거점 폐차
	TREE,													// 2거점 나무
	BARRICADE											// 3거점 엔지니어 스킬, 바리게이트
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

struct VectorBox												// 객체의 바운딩 박스 표현하기 위해 사용하는 vector4 구조체 
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

struct cs_login_packet									// 클라이언트에서 서버로 로그인 패킷 전송
{
	unsigned short size;									// 사이즈 전송
	MsgType type;										// 메시지 타입 LOGIN_REQUEST
};

struct cs_move_packet								// 클라이언트에서 서버로 움직이는 방향 전송
{
	unsigned short size;
	MsgType type;										// 메시지 타입 PLAYER_MOVE
	float t_x, t_z;											// 플레이어 방향
	float speed;											// 플레이어 스피드
	float x, z;												// 플레이어 좌표
	float rotation_angle;									// 플레이어 회전
	bool in_input;											// 플레이어 입력했는가?
};

struct cs_rotate_packet								// 클라이언트에서 서버로 플레이어가 회전한 마우스 위치를 전송
{
	unsigned short size;
	MsgType type;										// 메시지 타입 PLAYER_ROTATE
	float mx, mz;											// 플레이어 마우스 좌표
};

struct cs_chat_packet									// 클라이언트에서 서버로 채팅 메시지 전송
{
	unsigned short size;
	MsgType type;										// 메시지 타입 PLAYER_CHAT
	char message[MAX_CHAT_SIZE];				// 플레이어 채팅 메시지
};

struct cs_attack_packet								// 클라이언트에서 서버로 공격 방향 전송
{
	unsigned short size;
	MsgType type;										// 메시지 타입 PLYAER_ATTACK
	short x, z;												// 공격할 좌표(마우스 클릭 위치) x,z
	float mx, mz;											// 공격 방향
};

struct cs_build_packet									// 클라이언트에서 어느 위치에 무엇을 설치할 것인지 전송
{
	unsigned short size;
	MsgType type;										// 메시지 타입 PLAYER_BUILD
	float x, z;												// 설치할 위치
	ObjectType object;									// 오브젝트 종류
};

struct cs_interation_packet							// 오브젝트 상호작용 요청 패킷
{
	unsigned short size;
	MsgType type;										// 메시지 타입 PLAYER_INTERATION
	//char id;													// 오브젝트 아이디
	//ObjectType object;									// 오브젝트 종류
};

struct cs_select_packet								// 클라이언트에서 어떤 캐릭터를 선택할지 전송
{
	unsigned short size;
	MsgType type;										// 메시지 타입 PLAYER_SELECT
	PlayerType playertype;								// 고른 플레이어 종류
};

struct cs_request_packet									// 클라이언트에서 게임이 시작했는지 확인하고자 할 때 전송
{
	unsigned short size;
	MsgType type;										// 메시지 타입 CS_GAME_START_REQUEST
};

struct cs_special_req_packet						// 클라이언트에서 커맨더가 누굴 살릴지 요청하는 패킷
{
	unsigned short size;
	MsgType type;										// 메시지 타입 CS_SPECAIL_SKILL_REQUEST
	int id;														// 커맨더 살릴 ID
};

struct sc_login_ok_packet							// 서버에서 클라이언트에게 로그인이 되었다고 전송
{
	unsigned short size;
	MsgType type;										// 메시지 타입 LOGIN_OK
	char id;													// 클라이언트 ID 전송
	char select_type;										// 000 NONE, 001 COMMANDER, 010 ENGINEER, 100 MER
};

struct sc_login_other_packet
{
	unsigned short size;
	MsgType type;										// 메시지 타입 LOGIN_OK
	char id;													// 클라이언트 ID 전송
	char select_type;										// 000 NONE, 001 COMMANDER, 010 ENGINEER, 100 MER
};

struct sc_fail_packet						 			// 서버에서 클라이언트에게 로그인에 실패하였다고 전송
{
	unsigned short size;												
	MsgType type;										// 메시지 타임 LOGIN_FAIL, SELECT_FAIL
	char id;
};

struct sc_player_select_packet						// 서버에서 클라이언트에게 캐릭터 선택을 알려줌
{
	unsigned short size;
	MsgType type;										// 메시지 타입 PLAYER_SELECT_COMPLETE
	PlayerType playertype;								// 플레이어 타입 해당 캐릭터 
	char id;													// 플레이어 id
	float x, z;												// 플레이어 위치
	short hp, maxhp;
	char bullet;												// 플레이어 총알 보유 개수
	float speed;											// 플레이어 이동 속도
	char select_type;										// 000 NONE, 001 COMMANDER, 010 ENGINEER, 100 MER
};

struct sc_barricade_packet							// 서버에서 바리케이드 위치 보내주는 패킷
{
	unsigned short size;
	MsgType type;										// 메시지 타입 BARRICADE_SEND
	char id;
	iPos one_base[42];								// 첫번째 거점 미로 시작 위치, 방향
	iPos two_base[32];									// 두번째 거점 미로 시작 위치, 방향
	iPos three_base[30];								// 세번째 거점1 미로 시작 위치, 방향
	iPos three_base2[30];								// 세번째 거점2 미로 시작 위치, 방향
};

struct sc_game_start_packet						// 서버에서 게임이 시작하면 보내주는 패킷
{
	unsigned short size;
	MsgType type;										// 메시지 타입 GAME_START
	char id;
	// 아마 시작 대사 들어가야 할거 같음
};

struct sc_player_idle_packet							// 서버에서 플레이어 가만히 있다고 보내주는 패킷
{
	unsigned short size;
	MsgType type;										// 메시지 타입 PLAYER_IDLE
	char id;
};

struct sc_player_move_packet						// 서버에서 플레이어가 이동하면 보내주는 패킷
{
	unsigned short size;
	MsgType type;										// 메시지 타입 PLAYER_MOVE
	char id;
	float x, z;												// 플레이어 위치
	float t_x, t_z;											// 플레이어 방향
	float speed;
	float rotation_angle;									// 플레이어 회전
	bool in_input;											// 플레이어 입력했는가?
};

struct sc_player_rotate_packet								// 서버에서 플레이어가 회전한 마우스 위치를 전송
{
	unsigned short size;
	MsgType type;										// 메시지 타입 PLAYER_ROTATE
	char id;
	float mx, mz;											// 플레이어 마우스 좌표
};

struct sc_player_chat_packet						// 서버에서 모든 클라에게 채팅 메시지를 전달
{
	unsigned short size;
	MsgType type;										// 메시지 타입 PLAYER_CHAT
	char id;
	char s_id;												// 누가 보냈는지
	char message[MAX_CHAT_SIZE];				// 모든 아이디에게 보낼 아이디
};

struct sc_player_build_packet						// 서버에서 클라이언트에게 어느 오브젝트를 위치에 설치했는지 전달하는 패킷
{
	unsigned short size;
	MsgType type;										// 메시지 타입 PLAYER_BUILD
	char id[MAX_PLAYER];
	float x, z;												// 설치한 오브젝트 위치
	ObjectType object;									// 설치한 오브젝트 종류
};

struct sc_player_reload_packet						// 서버에서 재장전 했을 때 보내줌
{
	unsigned short size;
	MsgType type;										// 메시지 타입 PlayerReload
	char id;
};

struct sc_player_interation_packet					// 서버에서 클라이언트에게 플레이어 상호작용 패킷
{
	unsigned short size;
	MsgType type;										// 메시지 타입 PLAYER_INTERACTION
	char id[MAX_PLAYER];
	ObjectState objstate;								// 오브젝트 상태 부서져있는, 조금 부서진, 멀쩡한
};

struct sc_player_infection_packet					// 서버에서 클라이언트에게 감염도를 보내주는 패킷
{
	unsigned short size;
	MsgType type;										// 메시지 타입 PLYAER_INFECTION
	char id;													// 감염도는 본인만 알고 있어도 됨
	float infection;										// 감염도
};

struct sc_player_special_packet					// 서버에서 클라이언트에게 특수능력 사용 했는지 알려주는 패킷
{
	unsigned short size;
	MsgType type;										// 메시지 타입 PLAYER_SPECIAL
	char id;													// 해당 클라만 알고있으면 됨
	bool success;											// 발동 했는지 안했는지
};

struct sc_player_co_special_check_packet		// 커맨더 특수능력 사용 여부 체크하는 패킷
{
	unsigned short size;
	MsgType type;										// SPECIAL_CHECK
	char id;													// 누구에게 할 건지?
};

struct sc_commander_special_packet			// 서버에서 커맨더가 특수능력 사용했다고 알려주는 패킷
{
	unsigned short size;
	MsgType type;										// 메시지 타입 COMMANDER_SPECIAL
	char id;													// 살려낸 ID
	short hp;												// 해당 ID 의 체력
	char bullet;												// 해당 ID 의 총알 수
};

struct sc_player_en_special_check_packet		// 엔지니어 특수능력 사용 여부 체크하는 패킷
{
	unsigned short size;
	MsgType type;										// SPECIAL_CHECK
	short x, z;												// 어디에 지을거냐
};

struct sc_engineer_barrigate_build_packet		// 서버에서 엔지니어가 특수능력 사용했다고 알려주는 패킷
{
	unsigned short size;
	MsgType type;										// 메시지 타입 ENGINEER_SPECIAL
	char id;
	short x, z;												// 바리게이트 중심 위치
	Direction dir;											// 방향
};

struct sc_player_attack_packet						// 서버에서 클라이언트에게 누가 공격하고 있는지 알려주기
{
	unsigned short size;
	MsgType type;										// 메시지 타입 PLAYER_ATTACK
	char id;
	float mx, mz;
};

struct sc_player_dead_packet						// 서버에서 클라이언트에게 클라이언트가 죽었다고 보내기
{
	unsigned short size;
	MsgType type;										// 메시지 타입 PLAYER_DEAD
	char id;													// 죽은 클라이언트 ID
};

struct sc_player_hp_packet							// 서버에서 클라이언트에게 클라이언트 HP가 몇 남았는지 보내기
{
	unsigned short size;
	MsgType type;										// 메시지 타입 UPDATE_PLAYER_INFO
	char id;													// 해당 ID의 hp
	short hp;												// 클라이언트의  hp양
};

struct sc_player_zombie_klil_packet				// 서버에서 클라에게 이 클라가 좀비 몇마리 죽였다고 알려주는 패킷
{
	unsigned short size;
	MsgType type;										// 메시지 타입 ZOMBIE_KILL_NUM
	char id;													// 누가?
	short zom_num;										// 죽인 좀비 숫자.
};

struct sc_zombie_spawn_packet					// 서버에서 클라이언트에게 좀비의 종류와 스폰 위치를 알려주는 패킷
{
	unsigned short size;
	MsgType type;										// 메시지 타입 ZOMBIE_SPAWN
	MapType map_type;
	ZombieType zomtype;								// 좀비 종류 전송
	unsigned char id;
	float x, z;												// 좀비 스폰 위치
	ZombieState state;
	char hp;													// 좀비 hp
	float angle;												// 좀비가 향하는 방향
};

struct sc_zombie_move_packet						// 서버에서 클라이언트에게 좀비의 위치 전달
{
	unsigned short size;
	MsgType type;										// 메시지 타입 ZOMBIE_MOVE
	MapType map_type;								// 현재 존재하는 맵 위치
	unsigned char id;									// 좀비의 ID
	float x, z;												// 좀비가 이동한 위치
	float t_x, t_z;											// 좀비의 방향
	float speed;											// 좀비의 스피드
	Direction dir;											// 좀비 방향
};

struct sc_zombie_arrive_packet
{
	unsigned short size;
	MsgType type;										// 메시지 타입 ZOMBIE_ARRIVE
	MapType map_type;								// 좀비 위치
	unsigned char id;									// 어떤 ID가 도착했는가
	Direction dir;											// 공격 방향
};

struct sc_zombie_attack_packet					// 서버에서 좀비가 공격을 하면 전달
{
	unsigned short size;
	MsgType type;										// 메시지 타입 ZOMBIE_ATTACK
	unsigned char id;
	MapType m_type;									// 맵종류
};

struct sc_zombie_search_packet					// 서버에서 좀비가 플레이어를 발견하면 전달
{
	unsigned short size;
	MsgType type;										// 메시지 타입 ZOMBIE_SEARCH
	unsigned char z_id;									// 발견한좀비의 ID
	char p_id;												// 발견당한 플레이어의 ID
	MapType m_type;									// 맵종류
};

struct sc_zombie_mad_packet						// 서버에서 클라이언트에게 좀비가 광폭화하면 전달
{
	unsigned short size;
	MsgType type;										// 메시지 타입 ZOMBIE_MAD
	unsigned char id;									// 어떤 좀비가 광폭화 했는가?
};

struct sc_zombie_viewlist_packet					// 서버에서 클라이언트에게 뷰리스트 보낼 때 사용
{
	unsigned short size;
	MsgType type;										// 넣으면 ZOMBIE_VIEWLIST_PUT , 빼면 ZOMBIE_VIEWLIST_REMOVE
	char id;													// 어떤 클라이언트에 보내는지
	unsigned char z_id;									// 넣고 뺄 좀비
	MapType m_type;									// 위치한 맵 위치
	float x, z;												// 좀비 위치
	MsgType animation;								// 좀비 애니메이션
	ZombieType z_type;								// 좀비 타입
};

struct sc_zombie_dead_packet						// 서버에서 클라이언트에게 좀비가 죽었다고 전달
{
	unsigned short size;
	MsgType type;										// 메시지 타입 ZOMBIE_DEAD
	unsigned char id;
	MapType m_type;									// 어느 맵에서 죽었나
};

struct sc_zombie_num_packet						// 서버에서 남은 좀비가 몇마리인지 보내는 패킷
{
	unsigned short size;
	MsgType type;										// 메시지 타입 ZOMBIE_NUM
	unsigned char zombie_num;						// 남은 좀비 마릿수
};

struct sc_zombie_all_kill_packet					// 서버에서 좀비가 다 죽었다고 보내는 패킷
{
	unsigned short size;
	MsgType type;										// 메시지 타입 ZOMBIE_ALL_KILL
	MapType m_type;									// 죽은 맵 타입
};

struct sc_update_zombie_info_packet			// 서버에서 변한 좀비의 정보를 전달
{
	unsigned short size;
	MsgType type;										// 메시지 타입 UPDATE_ZOMBIE_INFO
	unsigned char id;									// 정보가 변한 좀비의 아이디
	char hp;													// 좀비의 변한 hp
	MapType map_type;								// 좀비 위치
};

struct sc_update_object_info_packet				// 서버에서 클라이언트에게 오브젝트 정보 전달
{
	unsigned short size;
	MsgType type;										// 메시지 타입 UPDATE_OBJECT_INFO
	char id;													// 오브젝트 ID
	char hp;													// 오브젝트 HP
	ObjectState state;									// 오브젝트 상태
};

struct sc_win_state_packet							// 서버에서 클라이언트에게 게임 결과 상태 알려주기
{
	unsigned short size;
	MsgType type;										// 메시지 타입 WIN_STATE
	WinGameState state;								// 게임 상태 확인
};

struct sc_search_packet								// 서버에서 클라이언트가 문 또는 오브젝트 등을 발견했을 때 내용 전달
{
	unsigned short size;
	MsgType type;										// 메시지 타입 SC_PLAYER_SEARCH
	char id;
	ObjectType obj_type;								// 발견한 오브젝트 타입
	short x, z;												// 발견한 오브젝트의 위치
};

struct sc_door_open_packet
{
	unsigned short size;
	MsgType type;										// 메시지 타입 SC_DOOR_OPEN
	char id;
	short row, col;											// 문 위치
	short size_x, size_z;									// 문 크기
};

struct sc_wait_packet
{
	unsigned short size;
	MsgType type;
	char id;
};

struct sc_gm_change_map_packet				// 서버에서 GM 명령으로 맵 변경 되었다고 전하는 패킷
{
	unsigned short size;
	MsgType type;										// GM_MAP_CHANGE_맵종류
	char id;													// 어떤 ID 가 이동했냐?
	short x, z;												// x, y 좌표 설정
};

struct sc_gm_player_hp_packet						// 서버에서 GM 명령으로 플레이어의 HP를 회복시키는 패킷
{
	unsigned short size;
	MsgType type;										// GM_PLAYER_HP_UP
	char id;													// 어떤 ID가 오름?
	short hp;												// 오른 HP
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