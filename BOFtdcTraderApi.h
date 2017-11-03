/////////////////////////////////////////////////////////////////////////
///@author yungchaan jiu
///@file BOFtdcTraderApi.h
///@
/////////////////////////////////////////////////////////////////////////

#ifndef __COX_FTDCTRADERAPI_H__
#define __COX_FTDCTRADERAPI_H__

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "SecurityFtdcUserApiStruct.h"

class BOFtdcTraderSpi
{
public:
	///µ±¿Í»§¶ËÓë½»Ò×ºóÌ¨½¨Á¢ÆðÍ¨ÐÅÁ¬½ÓÊ±£¨»¹Î´µÇÂ¼Ç°£©£¬¸Ã·½·¨±»µ÷ÓÃ¡£
	virtual void OnFrontConnected(){};
	
	///µ±¿Í»§¶ËÓë½»Ò×ºóÌ¨Í¨ÐÅÁ¬½Ó¶Ï¿ªÊ±£¬¸Ã·½·¨±»µ÷ÓÃ¡£µ±·¢ÉúÕâ¸öÇé¿öºó£¬API»á×Ô¶¯ÖØÐÂÁ¬½Ó£¬¿Í»§¶Ë¿É²»×ö´¦Àí¡£
	///@param nReason ´íÎóÔ­Òò
	///        0x1001 ÍøÂç¶ÁÊ§°Ü
	///        0x1002 ÍøÂçÐ´Ê§°Ü
	///        0x2001 ½ÓÊÕÐÄÌø³¬Ê±
	///        0x2002 ·¢ËÍÐÄÌøÊ§°Ü
	///        0x2003 ÊÕµ½´íÎó±¨ÎÄ
	virtual void OnFrontDisconnected(int nReason){};
		
	///ÐÄÌø³¬Ê±¾¯¸æ¡£µ±³¤Ê±¼äÎ´ÊÕµ½±¨ÎÄÊ±£¬¸Ã·½·¨±»µ÷ÓÃ¡£
	///@param nTimeLapse ¾àÀëÉÏ´Î½ÓÊÕ±¨ÎÄµÄÊ±¼ä
	virtual void OnHeartBeatWarning(int nTimeLapse){};	

	///´íÎóÓ¦´ð
	virtual void OnRspError(CSecurityFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {};

	///µÇÂ¼ÇëÇóÏìÓ¦
	virtual void OnRspUserLogin(CSecurityFtdcRspUserLoginField *pRspUserLogin, CSecurityFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {};

	///µÇ³öÇëÇóÏìÓ¦
	virtual void OnRspUserLogout(CSecurityFtdcUserLogoutField *pUserLogout, CSecurityFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {};

	///»ñÈ¡ÈÏÖ¤Ëæ»úÂëÇëÇóÏìÓ¦
	virtual void OnRspFetchAuthRandCode(CSecurityFtdcAuthRandCodeField *pAuthRandCode, CSecurityFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {};

	///±¨µ¥Â¼ÈëÇëÇóÏìÓ¦
	virtual void OnRspOrderInsert(CSecurityFtdcInputOrderField *pInputOrder, CSecurityFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {};

	///±¨µ¥²Ù×÷ÇëÇóÏìÓ¦
	virtual void OnRspOrderAction(CSecurityFtdcInputOrderActionField *pInputOrderAction, CSecurityFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {};

	// ///ÓÃ»§¿ÚÁî¸üÐÂÇëÇóÏìÓ¦
	// virtual void OnRspUserPasswordUpdate(CSecurityFtdcUserPasswordUpdateField *pUserPasswordUpdate, CSecurityFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {};

	// ///×Ê½ðÕË»§¿ÚÁî¸üÐÂÇëÇóÏìÓ¦
	// virtual void OnRspTradingAccountPasswordUpdate(CSecurityFtdcTradingAccountPasswordUpdateField *pTradingAccountPasswordUpdate, CSecurityFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {};

	///±¨µ¥Í¨Öª
	virtual void OnRtnOrder(CSecurityFtdcOrderField *pOrder) {};

	///³É½»Í¨Öª
	virtual void OnRtnTrade(CSecurityFtdcTradeField *pTrade) {};

	///±¨µ¥Â¼Èë´íÎó»Ø±¨
	virtual void OnErrRtnOrderInsert(CSecurityFtdcInputOrderField *pInputOrder, CSecurityFtdcRspInfoField *pRspInfo) {};

	///±¨µ¥²Ù×÷´íÎó»Ø±¨
	virtual void OnErrRtnOrderAction(CSecurityFtdcOrderActionField *pOrderAction, CSecurityFtdcRspInfoField *pRspInfo) {};

	// /Æ½Ì¨×´Ì¬ÐÅÏ¢Í¨Öª
	// virtual void OnRtnPlatformStateInfo(CSecurityFtdcPlatformStateInfoField *pPlatformStateInfo) {};
};

#ifndef WINDOWS
	#if __GNUC__ >= 4
		#pragma GCC visibility push(default)
	#endif
#endif

class BOFtdcTraderApi
{
public:
	///´´½¨TraderApi
	///@param pszFlowPath ´æÖü¶©ÔÄÐÅÏ¢ÎÄ¼þµÄÄ¿Â¼£¬Ä¬ÈÏÎªµ±Ç°Ä¿Â¼
	///@return ´´½¨³öµÄUserApi
	static BOFtdcTraderApi *CreateFtdcTraderApi(const char *pszFlowPath = "");
	
	///É¾³ý½Ó¿Ú¶ÔÏó±¾Éí
	///@remark ²»ÔÙÊ¹ÓÃ±¾½Ó¿Ú¶ÔÏóÊ±,µ÷ÓÃ¸Ãº¯ÊýÉ¾³ý½Ó¿Ú¶ÔÏó
	virtual void Release() = 0;
	
	///³õÊ¼»¯
	///@remark ³õÊ¼»¯ÔËÐÐ»·¾³,Ö»ÓÐµ÷ÓÃºó,½Ó¿Ú²Å¿ªÊ¼¹¤×÷
	virtual void Init() = 0;
	
	///µÈ´ý½Ó¿ÚÏß³Ì½áÊøÔËÐÐ
	///@return Ïß³ÌÍË³ö´úÂë
	virtual int Join() = 0;
	
	///»ñÈ¡µ±Ç°½»Ò×ÈÕ
	///@retrun »ñÈ¡µ½µÄ½»Ò×ÈÕ
	///@remark Ö»ÓÐµÇÂ¼³É¹¦ºó,²ÅÄÜµÃµ½ÕýÈ·µÄ½»Ò×ÈÕ
	virtual const char *GetTradingDay() = 0;
	
	///×¢²áÇ°ÖÃ»úÍøÂçµØÖ·
	///@param pszFrontAddress£ºÇ°ÖÃ»úÍøÂçµØÖ·¡£
	///@remark ÍøÂçµØÖ·µÄ¸ñÊ½Îª£º¡°protocol://ipaddress:port¡±£¬Èç£º¡±tcp://127.0.0.1:17001¡±¡£ 
	///@remark ¡°tcp¡±´ú±í´«ÊäÐ­Òé£¬¡°127.0.0.1¡±´ú±í·þÎñÆ÷µØÖ·¡£¡±17001¡±´ú±í·þÎñÆ÷¶Ë¿ÚºÅ¡£
	virtual void RegisterFront(char *pszFrontAddress) = 0;
	
	///×¢²á»Øµ÷½Ó¿Ú
	///@param pSpi ÅÉÉú×Ô»Øµ÷½Ó¿ÚÀàµÄÊµÀý
	virtual void RegisterSpi(BOFtdcTraderSpi *pSpi) = 0;
	
	///¶©ÔÄË½ÓÐÁ÷¡£
	///@param nResumeType Ë½ÓÐÁ÷ÖØ´«·½Ê½  
	///        SECURITY_TERT_RESTART:´Ó±¾½»Ò×ÈÕ¿ªÊ¼ÖØ´«
	///        SECURITY_TERT_RESUME:´ÓÉÏ´ÎÊÕµ½µÄÐø´«
	///        SECURITY_TERT_QUICK:Ö»´«ËÍµÇÂ¼ºóË½ÓÐÁ÷µÄÄÚÈÝ
	///@remark ¸Ã·½·¨ÒªÔÚInit·½·¨Ç°µ÷ÓÃ¡£Èô²»µ÷ÓÃÔò²»»áÊÕµ½Ë½ÓÐÁ÷µÄÊý¾Ý¡£
	virtual void SubscribePrivateTopic(SECURITY_TE_RESUME_TYPE nResumeType) = 0;
	
	///¶©ÔÄ¹«¹²Á÷¡£
	///@param nResumeType ¹«¹²Á÷ÖØ´«·½Ê½  
	///        SECURITY_TERT_RESTART:´Ó±¾½»Ò×ÈÕ¿ªÊ¼ÖØ´«
	///        SECURITY_TERT_RESUME:´ÓÉÏ´ÎÊÕµ½µÄÐø´«
	///        SECURITY_TERT_QUICK:Ö»´«ËÍµÇÂ¼ºó¹«¹²Á÷µÄÄÚÈÝ
	///@remark ¸Ã·½·¨ÒªÔÚInit·½·¨Ç°µ÷ÓÃ¡£Èô²»µ÷ÓÃÔò²»»áÊÕµ½¹«¹²Á÷µÄÊý¾Ý¡£
	virtual void SubscribePublicTopic(SECURITY_TE_RESUME_TYPE nResumeType) = 0;

	///ÓÃ»§µÇÂ¼ÇëÇó
	virtual int ReqUserLogin(CSecurityFtdcReqUserLoginField *pReqUserLoginField, int nRequestID) = 0;
	

	///µÇ³öÇëÇó
	virtual int ReqUserLogout(CSecurityFtdcUserLogoutField *pUserLogout, int nRequestID) = 0;

	///»ñÈ¡ÈÏÖ¤Ëæ»úÂëÇëÇó
	virtual int ReqFetchAuthRandCode(CSecurityFtdcAuthRandCodeField *pAuthRandCode, int nRequestID) = 0;

	///±¨µ¥Â¼ÈëÇëÇó
	virtual int ReqOrderInsert(CSecurityFtdcInputOrderField *pInputOrder, int nRequestID) = 0;

	///±¨µ¥²Ù×÷ÇëÇó
	virtual int ReqOrderAction(CSecurityFtdcInputOrderActionField *pInputOrderAction, int nRequestID) = 0;

	// ///ÓÃ»§¿ÚÁî¸üÐÂÇëÇó
	// virtual int ReqUserPasswordUpdate(CSecurityFtdcUserPasswordUpdateField *pUserPasswordUpdate, int nRequestID) = 0;

	// ///×Ê½ðÕË»§¿ÚÁî¸üÐÂÇëÇó
	// virtual int ReqTradingAccountPasswordUpdate(CSecurityFtdcTradingAccountPasswordUpdateField *pTradingAccountPasswordUpdate, int nRequestID) = 0;

protected:
	~BOFtdcTraderApi(){};
};
  

#endif
