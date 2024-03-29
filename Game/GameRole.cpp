﻿#include "GameRole.h"
#include"GameMsg.h"
#include"AOIWorld.h"
#include<iostream>
#include"msg.pb.h"
#include"GameChannel.h"
#include"GameProtocol.h"
#include<random>
#include<algorithm>
#include"ZinxTimer.h"
#include"RandName.h"
#include<fstream>
#include<hiredis/hiredis.h>
using namespace std;
/*创建游戏世界的全局对象*/
RandName random_name;
static AOIWorld world(0, 400, 0, 400, 20, 20);
void GameRole::procMoveMsg(float _x, float _y, float _z, float _v)
{
	/*1.跨网格处理*/
		/*获取原来的邻居*/
	auto s1 = world.GetSrdPlayers(this);

	/*摘除旧格子，跟新坐标，添加新格子，获取新邻居s2*/
	world.DelPlayer(this);
	 x = _x;
	 y = _y;
	 z = _z;
	 v = _v;
	 world.AddPlayer(this);
	 auto s2 = world.GetSrdPlayers(this);

	 /*遍历s2,若元素不属于s1,视野出现*/
	 for (auto single : s2)
	 {
		 if (s1.end()==find(s1.begin(),s1.end(),single))
		 {
			 //视野出现
			 ViewAppear(dynamic_cast<GameRole*>(single));
		 }
	 }
	 /*遍历s1,若元素不属于s2,视野消失*/	
	 for (auto single : s1)
	 {
		 if (s2.end() == find(s2.begin(), s2.end(), single))
		 {
			 //视野出现
			 ViewLost(dynamic_cast<GameRole*>(single));
		 }
	 }
/*注释：s1为旧邻居的集合，s2为新邻居的集合*/

/* 2.广播新位置给周围玩家*/
	 auto srd_list = world.GetSrdPlayers(this);

	 for (auto single : srd_list)
	 {
		 //组成待发送的报文 
		 pb::BroadCast* pMsg = new pb::BroadCast();
		 auto pPos = pMsg->mutable_p();
		 pPos->set_x(_x);
		 pPos->set_y(_y);
		 pPos->set_z(_z);
		 pPos->set_v(_v);
		 pMsg->set_pid(iPid);
		 pMsg->set_tp(4);
		 pMsg->set_username(szName);
		 auto pRole = dynamic_cast<GameRole*>(single);
		 auto msg = new GameMsg(GameMsg::MSG_TYPE_BROADCAST, pMsg);
		 ZinxKernel::Zinx_SendOut(*msg, *(pRole->m_pProto));
	 }
}
void GameRole::ViewAppear(GameRole * _pRole)
{
	/*向自己发送参数200的消息*/
	auto pmsg = _pRole->createSelfPostion();
	ZinxKernel::Zinx_SendOut(*pmsg, *m_pProto);
	/*向参数玩家发送自己的200号消息*/
	pmsg = createSelfPostion();
	ZinxKernel::Zinx_SendOut(*pmsg, *(_pRole->m_pProto));

}
void GameRole::ViewLost(GameRole * _pRole)
{
	/*向自己发送参数201的消息*/
	auto pmsg = _pRole->createIDNameLogoff();
	ZinxKernel::Zinx_SendOut(*pmsg, *m_pProto);
	/*向参数玩家发送自己的201号消息*/
	pmsg = createIDNameLogoff();
	ZinxKernel::Zinx_SendOut(*pmsg, *(_pRole->m_pProto));
}
GameMsg * GameRole::createIDNameLogin()
{
	pb::SyncPid* pmsg = new pb::SyncPid();
	pmsg->set_pid(iPid);
	pmsg->set_username(szName);
	GameMsg* pRet = new GameMsg(GameMsg::MSG_TYPE_LOGIN_ID_NAME, pmsg);
	return pRet;
}

GameMsg * GameRole::createIDNameLogoff()
{

	pb::SyncPid* pmsg = new pb::SyncPid();
	pmsg->set_pid(iPid);
	pmsg->set_username(szName);
	GameMsg* pRet = new GameMsg(GameMsg::MSG_TYPE_LOGOFF_ID_NAME, pmsg);
	return pRet;
}

GameMsg * GameRole::createSrdPlayers()
{
	pb::SyncPlayers* pMsg = new pb::SyncPlayers();
	auto srd_list = world.GetSrdPlayers(this);
	for (auto single : srd_list)
	{
		auto  pPlayer = pMsg->add_ps();
		auto pRole = dynamic_cast<GameRole*>(single);
		pPlayer->set_pid(pRole->iPid);
		pPlayer->set_username(pRole->szName);
		auto pPostion = pPlayer->mutable_p();
		pPostion->set_x(pRole->x);
		pPostion->set_y(pRole->y);
		pPostion->set_z(pRole->z);
		pPostion->set_v(pRole->v);
	}
	GameMsg*  pret = new GameMsg(GameMsg::MSG_TYPE_SRD_POSITION, pMsg);
	return pret;
}

GameMsg * GameRole::createSelfPostion()
{
	pb::BroadCast* pMsg = new pb::BroadCast();
	pMsg->set_pid(iPid);
	pMsg->set_username(szName);
	pMsg->set_tp(2);
	 
	auto pPositon = pMsg->mutable_p();
	pPositon->set_x(x);
	pPositon->set_y(y);
	pPositon->set_z(z);
	pPositon->set_v(v);

	GameMsg*  pret = new GameMsg(GameMsg::MSG_TYPE_BROADCAST, pMsg);
	return pret;
}

GameMsg * GameRole::createTalkBroadCast(std::string _content)
{
	pb::BroadCast* pmsg = new pb::BroadCast();
	pmsg->set_pid(iPid);
	pmsg->set_username(szName);
	pmsg->set_tp(1);
	pmsg->set_content(_content);
	GameMsg* pRet = new GameMsg(GameMsg::MSG_TYPE_BROADCAST, pmsg);
	return pRet;
}
static default_random_engine random_engine(time(NULL));
GameRole::GameRole()
{

	//szName = "Tom";
	szName = random_name.GetName();
	  x =100+random_engine()%50;
	  z =100+random_engine() % 50;

}


GameRole::~GameRole()
{
	random_name.Rlease(szName);
}
class ExitTimer :public TimerOutProc
{
	// 通过 TimerOutProc 继承
	virtual void Proc() override
	{
		ZinxKernel::Zinx_Exit();
	}
	virtual int GetTImeSec() override
	{
		return 20;
	}
};
static ExitTimer g_exit_timer;
bool GameRole::Init()
{
	if (ZinxKernel::Zinx_GetAllRole().size() <= 0)
	{
		//启动定时器
		TimerOutMng::GetInstance().DelTask(&g_exit_timer);
	}
	/*添加到自己到游戏世界*/
	bool bRet = false;
	/*设置玩家ID为当前链接的fd*/
	iPid = m_pProto->m_channel->GetFd();
	bRet = world.AddPlayer(this);
	if (true==bRet)
	{
		/*向自己发送ID和名称*/
		auto pmsg = createIDNameLogin();
		ZinxKernel::Zinx_SendOut(*pmsg, *m_pProto);
		/*向自己发送周围玩家的位置*/	
		pmsg = createSrdPlayers();
		ZinxKernel::Zinx_SendOut(*pmsg, *m_pProto);
		/*向周围玩家发送自己的位置*/
		auto srd_list = world.GetSrdPlayers(this);
		for (auto single : srd_list)
		{
			pmsg = createSelfPostion();
			auto pRole = dynamic_cast<GameRole*>(single);
			ZinxKernel::Zinx_SendOut(*pmsg, *(pRole->m_pProto));
		}

	}

	//1通过写入文件的形式
	////以追加的形式记录当前姓名到文件
	//ofstream name_record("/tmp/name_record",ios::app);
	//name_record << szName << endl;
	//2.通过redis
	redisContext* context= redisConnect("127.0.0.1", 6379);
	if (NULL!=context)
	{
		freeReplyObject(redisCommand(context, "lpush game_name %s", szName.c_str()));
		redisFree(context);
	}
	return bRet;
}
/*处理游戏相关的用户请求*/
UserData * GameRole::ProcMsg(UserData & _poUserData)
{
	/*测设：打印信息内容*/
	GET_REF2DATA(MultiMsg, input, _poUserData);
	for (auto single : input.m_Msgs)
	{
		std::cout << "type is " << single->enMsgType <<std::endl;
		std::cout << single->pMsg->Utf8DebugString() << std::endl;
		/*聊天信息处理*/
		if (single->enMsgType==GameMsg::MSG_TYPE_CHAT_CONTENT)
		{
			/*取出聊天内容*/
			auto content = dynamic_cast<pb::Talk*>(single->pMsg)->content();
			/*发给所有人*/
			auto role_list = ZinxKernel::Zinx_GetAllRole();
			for (auto pRole:role_list)
			{
				auto pGameRole = dynamic_cast<GameRole*>(pRole);
				auto pmsg = createTalkBroadCast(content);
				ZinxKernel::Zinx_SendOut(*pmsg, *(pGameRole->m_pProto));
			}
		}
		/*向周围玩家发送自己位置*/
		if (single->enMsgType == GameMsg::MSG_TYPE_NEW_POSITION)
		{
			/*取出新位置*/
			auto NewPos = dynamic_cast<pb::Position*>(single->pMsg);
			/*遍历周围的玩家发送*/
			/*向周围玩家发送自己的位置*/
			procMoveMsg(NewPos->x(), NewPos->y(), NewPos->z(), NewPos->v());
		}
	}
	return nullptr;
}

void GameRole::Fini()
{
	
	/*向周围玩家发送自己的位置*/
	auto srd_list = world.GetSrdPlayers(this);
	for (auto single : srd_list)
	{
		GameMsg* pmsg = createIDNameLogoff();
		auto pRole = dynamic_cast<GameRole*>(single);
		ZinxKernel::Zinx_SendOut(*pmsg, *(pRole->m_pProto));
	}
	world.DelPlayer(this);
	/*判断是否是最后一个玩家---》起定时器*/
	if (ZinxKernel::Zinx_GetAllRole().size()<=1)
	{
		//启动定时器
		TimerOutMng::GetInstance().AddTask(&g_exit_timer);
	}
	//从文件中删除当前姓名
	//1.c从文件中读取姓名
	//list<string> name_list;
	//ifstream input("/tmp/name_record");
	//string tmp;
	//while (getline(input,tmp))
	//{
	//	name_list.push_back(tmp);
	//}
	////2.删除 当前姓名
	////3.写入其余姓名
	//ofstream output("/ tmp / name_record", ios::app);
	//for (auto namelisg : name_list)
	//{
	//	if (namelisg!=szName)
	//	{
	//		output << namelisg;
	//	}
	//}

	//redis
	redisContext* context = redisConnect("127.0.0.1", 6379);
	if (NULL != context)
	{
		freeReplyObject(redisCommand(context, "lrem game_name 1 %s", szName.c_str()));

		redisFree(context);
	}

}

int GameRole::GetX()
{
	return (int)x;
}

int GameRole::GetY()
{
	return (int)z;
}
