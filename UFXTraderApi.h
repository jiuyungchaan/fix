//
// Created by ht on 18-12-3.
//

#ifndef UFXADAPTER_UFXTRADERAPI_H
#define UFXADAPTER_UFXTRADERAPI_H

#include <t2sdk_interface.h>
#include <SecurityFtdcUserApiStruct.h>
#include <iostream>
#include <NHUserApiStruct.h>
#include <SecurityFtdcTraderApi.h>
#include "ufx_utils.h"

#ifdef UFX

enum {
    REQ_CLIENT_LOGIN = 331100,  ///客户登录
    REQ_ORDER_INSERT = 333002,  ///普通委托
    REQ_ORDER_ACTION = 333017,
    QRY_ORDER = 333101,
    QRY_TRADE = 333102,
    QRY_POSITION_FAST = 333103,
    QRY_POSITION = 333104,
    QRY_MONEY_FAST = 332254,
    QRY_MONEY = 332255,
    MSGCENTER_FUNC_HEART = 620000,
    MSGCENTER_FUNC_REG = 620001,
    MSGCENTER_FUNC_SENDED = 620003,


    ISSUE_TYPE_REALTIME_SECU = 12,                                              // 订阅证券成交回报
    ISSUE_TYPE_ENTR_BACK = 23                                              //订阅委托回报

};

#endif

#if defined(ISLIB) && defined(WIN32)
#ifdef LIB_TRADER_API_EXPORT
#define TRADER_API_EXPORT __declspec(dllexport)
#else
#define TRADER_API_EXPORT __declspec(dllimport)
#endif
#else
#define TRADER_API_EXPORT
#endif

class UFXTraderSpi : public CSecurityFtdcTraderSpi {
public:
    ///当客户端与交易后台建立起通信连接时（还未登录前），该方法被调用。
    virtual void OnFrontConnected() {};

    ///当客户端与交易后台通信连接断开时，该方法被调用。当发生这个情况后，API会自动重新连接，客户端可不做处理。
    ///@param nReason 错误原因
    ///        0x1001 网络读失败
    ///        0x1002 网络写失败
    ///        0x2001 接收心跳超时
    ///        0x2002 发送心跳失败
    ///        0x2003 收到错误报文
    virtual void OnFrontDisconnected(int nReason) {};

    ///心跳超时警告。当长时间未收到报文时，该方法被调用。
    ///@param nTimeLapse 距离上次接收报文的时间
    virtual void OnHeartBeatWarning(int nTimeLapse) {};

    ///登录请求响应
    virtual void
    OnRspUserLogin(CSecurityFtdcRspUserLoginField *pRspUserLogin, CSecurityFtdcRspInfoField *pRspInfo, int nRequestID,
                   bool bIsLast) {};

    ///登出请求响应
    virtual void
    OnRspUserLogout(CSecurityFtdcUserLogoutField *pUserLogout, CSecurityFtdcRspInfoField *pRspInfo, int nRequestID,
                    bool bIsLast) {};

    ///报单录入请求响应
    virtual void
    OnRspOrderInsert(CSecurityFtdcInputOrderField *pOrder, CSecurityFtdcRspInfoField *pRspInfo, int nRequestID,
                     bool bIsLast) {};

    ///报单操作请求响应
    virtual void
    OnRspOrderAction(CSecurityFtdcOrderActionField *pOrderAction, CSecurityFtdcRspInfoField *pRspInfo,
                     int nRequestID, bool bIsLast) {};

    ///报单通知
    virtual void OnRtnOrder(CSecurityFtdcOrderField *pOrder) {};

    ///成交通知
    virtual void OnRtnTrade(CSecurityFtdcTradeField *pTrade) {};

    virtual void
    OnRspQryTradingAccount(CSecurityFtdcTradingAccountField *pTradingAccount, CSecurityFtdcRspInfoField *pRspInfo,
                           int nRequestID, bool bIsLast) {
        std::cout << pTradingAccount->Balance << std::endl;

    }

    virtual void
    OnRspQryInvestorPosition(CSecurityFtdcQryInvestorPositionField *positionField, CSecurityFtdcRspInfoField *pRspInfo,
                             int nRequestID, bool bIsLast) {
        std::cout << positionField->InstrumentID << std::endl;
    }
};

class Callback;

class TRADER_API_EXPORT UFXTraderApi {
    friend class Callback;

private:
    UFXTraderSpi *_spi;

    CConnectionInterface *_lpConn;
    CCallbackInterface *_lpCallback;

    int _requestID;
    // login returned info
    const char _entrustWay;
    char _op_station[256];
    int _branch_no;
    int _sysnode_id;
    char _client_id[18];
    char _password[10];
    char _user_token[40];
    char _fund_account[18];


    void SubscribeErrorRsp() {}

public:
    static UFXTraderApi *CreateFtdcTraderApi(const char *) {
        return new UFXTraderApi();
    }

    UFXTraderApi() : _spi(NULL), _lpConn(NULL), _lpCallback(NULL), _entrustWay('Z') {
        //todo init op_station
//        char mac[64];
//        char ip[40];
//        char* remote="192.168.0.1:22";
//        GetLocalMACIP(mac,ip, remote);
        strcpy(_op_station, "PC;IIP:183.54.42.114;MAC:4cedfb65b36b;HD:50026B77824E95AE;@Octopus|V1.0.0");
    }

    ~UFXTraderApi() {
        _lpConn->Close();
        _lpConn->Release();
    }

    void
    LoginSetup(int branch_no, int sysnode_id, const char *client_id, const char *user_token, const char *fund_account) {
        _branch_no = branch_no;
        _sysnode_id = sysnode_id;
        strcpy(_client_id, client_id);
        strcpy(_user_token, user_token);
        strcpy(_fund_account, fund_account);
    }

    ///创建TraderApi
    ///@return 创建出的UserApi
    static UFXTraderApi *CreateFtdcTraderApi();

    ///删除接口对象本身
    ///@remark 不再使用本接口对象时,调用该函数删除接口对象
    virtual void Release();

    ///初始化
    ///@remark 初始化运行环境,只有调用后,接口才开始工作
    void Init(const char *licPath = NULL);

    ///等待接口线程结束运行
    ///@return 线程退出代码
    virtual int Join();

    ///获取当前交易日
    ///@retrun 获取到的交易日
    ///@remark 只有登录成功后,才能得到正确的交易日
    virtual const char *GetTradingDay();

    ///注册回调接口
    ///@param pSpi 派生自回调接口类的实例
    virtual void RegisterSpi(UFXTraderSpi *pSpi);

    ///用户登录请求
    virtual int ReqUserLogin(CSecurityFtdcReqUserLoginField *pReqUserLoginField, int nRequestID);

    int SubscribeOrderRsp();

    int SubscribeTradeRsp();

    ///登出请求
    virtual int ReqUserLogout(CSecurityFtdcUserLogoutField *pUserLogout, int nRequestID);

    ///报单录入请求
    virtual int ReqOrderInsert(CSecurityFtdcInputOrderField *pInputOrder, int nRequestID);

    ///报单操作请求
    virtual int ReqOrderAction(CSecurityFtdcInputOrderActionField *pInputOrderAction, int nRequestID);

///请求查询资金账户
    virtual int ReqQryTradingAccount(CSecurityFtdcQryTradingAccountField *pQryTradingAccount, int nRequestID);

///请求查询投资者持仓
    virtual int ReqQryInvestorPosition(CSecurityFtdcQryInvestorPositionField *pQryInvestorPosition, int nRequestID);
};


#endif //UFXADAPTER_UFXTRADERAPI_H
