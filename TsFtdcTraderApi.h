////////////////////////
///@author Kenny Chiu
///@date 20170120
///@summary A api for FtdTrader which excapsulated TS socket structures and logics
///         and transfer CTP structures to TS structures
///
////////////////////////


#ifndef __TS_FTDC_TRADER_API_H__
#define __TS_FTDC_TRADER_API_H__

// include CTP ThostFtdcUserApiStruct.h and transfer CTP structures to TS 
// structures
#include "ThostFtdcUserApiStruct.h"

class CTsFtdcTraderSpi {
 public:
  ///当客户端与交易后台建立起通信连接时（还未登录前），该方法被调用。
  virtual void OnFrontConnected(){};

  ///当客户端与交易后台通信连接断开时，该方法被调用。当发生这个情况后，API会自动重新连接，客户端可不做处理。
  ///@param nReason 错误原因
  ///        0x1001 网络读失败
  ///        0x1002 网络写失败
  ///        0x2001 接收心跳超时
  ///        0x2002 发送心跳失败
  ///        0x2003 收到错误报文
  virtual void OnFrontDisconnected(int nReason){};

  ///登录请求响应
  virtual void OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {};

  ///登出请求响应
  virtual void OnRspUserLogout(CThostFtdcUserLogoutField *pUserLogout, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {};

  ///报单录入请求响应
  virtual void OnRspOrderInsert(CThostFtdcInputOrderField *pInputOrder, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {};

  ///报单操作请求响应
  virtual void OnRspOrderAction(CThostFtdcInputOrderActionField *pInputOrderAction, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {};

  ///错误应答
  virtual void OnRspError(CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {};

  ///报单通知
  virtual void OnRtnOrder(CThostFtdcOrderField *pOrder) {};

  ///成交通知
  virtual void OnRtnTrade(CThostFtdcTradeField *pTrade) {};

  ///ÇëÇó²éÑ¯Í¶×ÊÕß³Ö²ÖÏìÓ¦
  virtual void OnRspQryInvestorPosition(CThostFtdcInvestorPositionField *pInvestorPosition, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {};

  ///ÇëÇó²éÑ¯×Ê½ðÕË»§ÏìÓ¦
  virtual void OnRspQryTradingAccount(CThostFtdcTradingAccountField *pTradingAccount, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {};

};

class CTsFtdcTraderApi {
 public:
  ///创建TraderApi
  ///@param pszFlowPath 存贮订阅信息文件的目录，默认为当前目录
  ///@return 创建出的UserApi
  static CTsFtdcTraderApi *CreateFtdcTraderApi(const char *pszFlowPath = "");

  ///初始化
  ///@remark 初始化运行环境,只有调用后,接口才开始工作
  virtual void Init() = 0;
  
  ///等待接口线程结束运行
  ///@return 线程退出代码
  virtual int Join() = 0;

  ///注册前置机网络地址
  ///@param pszFrontAddress：前置机网络地址。
  ///@remark 网络地址的格式为：“protocol://ipaddress:port”，如：”tcp://127.0.0.1:17001”。 
  ///@remark “tcp”代表传输协议，“127.0.0.1”代表服务器地址。”17001”代表服务器端口号。
  virtual void RegisterFront(char *pszFrontAddress) = 0;

  ///注册回调接口
  ///@param pSpi 派生自回调接口类的实例
  virtual void RegisterSpi(CTsFtdcTraderSpi *pSpi) = 0;

  ///订阅私有流。
  ///@param nResumeType 私有流重传方式  
  ///        THOST_TERT_RESTART:从本交易日开始重传
  ///        THOST_TERT_RESUME:从上次收到的续传
  ///        THOST_TERT_QUICK:只传送登录后私有流的内容
  ///@remark 该方法要在Init方法前调用。若不调用则不会收到私有流的数据。
  virtual void SubscribePrivateTopic(THOST_TE_RESUME_TYPE nResumeType) = 0;
  
  ///订阅公共流。
  ///@param nResumeType 公共流重传方式  
  ///        THOST_TERT_RESTART:从本交易日开始重传
  ///        THOST_TERT_RESUME:从上次收到的续传
  ///        THOST_TERT_QUICK:只传送登录后公共流的内容
  ///@remark 该方法要在Init方法前调用。若不调用则不会收到公共流的数据。
  virtual void SubscribePublicTopic(THOST_TE_RESUME_TYPE nResumeType) = 0;

  ///用户登录请求
  virtual int ReqUserLogin(CThostFtdcReqUserLoginField *pReqUserLoginField,
                           int nRequestID) = 0;
  
  ///登出请求
  virtual int ReqUserLogout(CThostFtdcUserLogoutField *pUserLogout,
                            int nRequestID) = 0;

  ///报单录入请求
  virtual int ReqOrderInsert(CThostFtdcInputOrderField *pInputOrder,
                             int nRequestID) = 0;

  ///报单操作请求
  virtual int ReqOrderAction(CThostFtdcInputOrderActionField *pInputOrderAction,
                             int nRequestID) = 0;

  ///ÇëÇó²éÑ¯Í¶×ÊÕß³Ö²Ö
  virtual int ReqQryInvestorPosition(CThostFtdcQryInvestorPositionField *pQryInvestorPosition, int nRequestID) = 0;

  ///ÇëÇó²éÑ¯×Ê½ðÕË»§
  virtual int ReqQryTradingAccount(CThostFtdcQryTradingAccountField *pQryTradingAccount, int nRequestID) = 0;

 protected:
  ~CTsFtdcTraderApi() {};
};

#endif  // __TS_FTDC_TRADER_API_H__