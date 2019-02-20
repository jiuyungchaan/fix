//
// Created by ht on 18-12-3.
//

#include "UFXTraderApi.h"
#include "UFXCallback.h"
#include "ufx_utils.h"

UFXTraderApi *UFXTraderApi::CreateFtdcTraderApi() {
    return new UFXTraderApi();
//    return nullptr;
}

void UFXTraderApi::RegisterSpi(UFXTraderSpi *pSpi) {
    _spi = pSpi;
    _lpCallback = new Callback(_spi, this);
}

void UFXTraderApi::Init(const char *licPath) {
    char mac[128];
    char ip[64];
    std::string traderName_;
//    if(GetLocalMACIP(mac,ip,serverAddr) == false)
//    {
//        return;
//    }
    CConfigInterface* lpConfig = NewConfig();
    lpConfig->AddRef();

    if (licPath)
        lpConfig->Load(licPath);
    else
        lpConfig->Load("t2sdk.ini");
    if (_lpConn != NULL) {
        _lpConn->Release();
        _lpConn = NULL;
    }
    _lpConn = NewConnection(lpConfig);
    _lpConn->AddRef();
    int iRet = _lpConn->Create2BizMsg(_lpCallback);
#ifndef NDEBUG
    if (0 == iRet) {
        std::cout << "InitConn lpConnection->Create2BizMsg(lpCallback) returns 0, OK." << std::endl;
//        CConnectionCenter::GetInstance()->ReConnect(this,true,false);
    } else {
        std::cout << "InitConn lpConnection->Create2BizMsg(lpCallback) returns " << iRet
                  << ", error. errorMsg=" << _lpConn->GetErrorMsg(iRet) << std::endl;
    }
#endif
    if (0 != (iRet = _lpConn->Connect(3000))) {
        std::cerr << "连接.iRet=" << iRet << " msg:" << _lpConn->GetErrorMsg(iRet) << std::endl;
    }
    lpConfig->Release();
}

int UFXTraderApi::ReqUserLogin(CSecurityFtdcReqUserLoginField *pReqUserLoginField, int nRequestID) {
    strcpy(_password, pReqUserLoginField->Password);
    IBizMessage *lpBizMessage = NewBizMessage();
    lpBizMessage->AddRef();
    lpBizMessage->SetFunction(REQ_CLIENT_LOGIN);
    lpBizMessage->SetPacketType(REQUEST_PACKET);
    IF2Packer *pPacker = NewPacker(2);
    if (!pPacker) {
        printf("取打包器失败！\r\n");
        return -1;
    }
    pPacker->AddRef();
    pPacker->BeginPack();
    ///加入字段名
    pPacker->AddField("op_branch_no", 'I');
    pPacker->AddField("op_entrust_way", 'C');
    pPacker->AddField("op_station", 'S', 255);
//    pPacker->AddField("branch_no", 'I');
    pPacker->AddField("operator_no", 'S', 15);
    pPacker->AddField("password", 'S', 15);
    pPacker->AddField("password_type", 'C');
    pPacker->AddField("input_content", 'C');
    pPacker->AddField("account_content", 'S', 30);
    pPacker->AddField("content_type", 'S', 6);

    pPacker->AddInt(0);//op branch no
    pPacker->AddChar(_entrustWay);
    pPacker->AddStr("op station");
//    pPacker->AddInt(_branch_no);//branch_no
    pPacker->AddStr("");
    pPacker->AddStr(pReqUserLoginField->Password);
    pPacker->AddChar('2');
    // pPacker->AddChar(pReqFuncClientLogin->input_content);
    pPacker->AddChar('1');
    pPacker->AddStr(pReqUserLoginField->UserID);
    pPacker->AddStr("");
    ///结束打包
    pPacker->EndPack();

    lpBizMessage->SetContent(pPacker->GetPackBuf(), pPacker->GetPackLen());

    IF2UnPacker *lpUnPacker = NewUnPacker(pPacker->GetPackBuf(), pPacker->GetPackLen());
    lpUnPacker->AddRef();
//    ShowPacket(lpUnPacker);
    lpUnPacker->Release();

    //异步模式登陆
    int iRet = _lpConn->SendBizMsg(lpBizMessage, 1);
#ifndef NDEBUG
    std::cout << "SendBizMsg: " << iRet << std::endl;
#endif
    if (iRet < 0) {
        std::cout << _lpConn->GetErrorMsg(iRet) << std::endl;
    }
    pPacker->FreeMem(pPacker->GetPackBuf());
    pPacker->Release();
    return 0;
}

const char *UFXTraderApi::GetTradingDay() {
    return NULL;
}


int UFXTraderApi::ReqOrderInsert(CSecurityFtdcInputOrderField *pInputOrder, int nRequestID) {
    int iRet = 0, hSend = 0;
    ///获取版本为2类型的pack打包器
    IF2Packer *pPacker = NewPacker(2);
    if (!pPacker) {
        printf("取打包器失败!\r\n");
        return -1;
    }
    pPacker->AddRef();

    ///定义解包器
    //IF2UnPacker *pUnPacker = NULL;

    IBizMessage *lpBizMessage = NewBizMessage();

    lpBizMessage->AddRef();

    ///应答业务消息
    IBizMessage *lpBizMessageRecv = NULL;
    //功能号
    lpBizMessage->SetFunction(REQ_ORDER_INSERT);
    lpBizMessage->SetSenderId(++_requestID);
    //请求类型
    lpBizMessage->SetPacketType(REQUEST_PACKET);
    //lpBizMessage->SetSystemNo(iSystemNo);
    ///其他的应答信息
    LPRET_DATA pRetData = NULL;
    ///开始打包
    pPacker->BeginPack();

    ///加入字段名
    pPacker->AddField("op_branch_no", 'I', 5);//名字 类型 长度
    pPacker->AddField("op_entrust_way", 'C', 1);
    pPacker->AddField("op_station", 'S', 255);
    pPacker->AddField("branch_no", 'I', 5);
    pPacker->AddField("client_id", 'S', 18);//客户ID
    pPacker->AddField("fund_account", 'S', 18);//资金账号
    pPacker->AddField("password", 'S', 10);
    pPacker->AddField("password_type", 'C', 1);
    pPacker->AddField("user_token", 'S', 40);
    pPacker->AddField("exchange_type", 'S', 4);
//    pPacker->AddField("stock_account", 'S', 15);
    pPacker->AddField("stock_code", 'S', 6);
    pPacker->AddField("entrust_amount", 'F', 19, 2);
    pPacker->AddField("entrust_price", 'F', 18, 3);
    pPacker->AddField("entrust_bs", 'C', 1);
    pPacker->AddField("entrust_prop", 'S', 3);
    pPacker->AddField("batch_no", 'I', 8);

    ///加入对应的字段值
    pPacker->AddInt(0);
    pPacker->AddChar(_entrustWay);
    pPacker->AddStr(_op_station);
    pPacker->AddInt(_branch_no);
    pPacker->AddStr(_client_id);
    pPacker->AddStr(_fund_account);
    pPacker->AddStr(_password);
    pPacker->AddChar('2');
    pPacker->AddStr(_user_token);
    pPacker->AddStr(pInputOrder->ExchangeID);
//    pPacker->AddStr("");
    pPacker->AddStr(pInputOrder->InstrumentID);
    pPacker->AddDouble(pInputOrder->VolumeTotalOriginal);
    double price;
    sscanf(pInputOrder->LimitPrice, "%lf", &price);
    pPacker->AddDouble(price);
    if (pInputOrder->Direction == SECURITY_FTDC_D_Buy)
        pPacker->AddChar('1');
    else if (pInputOrder->Direction == SECURITY_FTDC_D_Sell)
        pPacker->AddChar('2');

    pPacker->AddStr("0");
    int batchNo = atoi(pInputOrder->OrderRef);
    pPacker->AddInt(batchNo);

    pPacker->EndPack();
    lpBizMessage->SetContent(pPacker->GetPackBuf(), pPacker->GetPackLen());
#ifndef NDEBUG
    //打印入参信息
    IF2UnPacker *lpUnPacker = NewUnPacker(pPacker->GetPackBuf(), pPacker->GetPackLen());
    lpUnPacker->AddRef();
    printf("Order Packet:\n");
    ShowPacket(lpUnPacker);
    lpUnPacker->Release();
#endif
//    printf("%s %s\n", pInputOrder->OrderRef, pInputOrder->InstrumentID);
    this->request2OrderInsert[_requestID] = std::pair<int, std::string>(atoi(pInputOrder->OrderRef),
                                                                        pInputOrder->InstrumentID);
    iRet = _lpConn->SendBizMsg(lpBizMessage, 1);
    printf("返回代码：%d\n", iRet);
    pPacker->FreeMem(pPacker->GetPackBuf());
    pPacker->Release();
    lpBizMessage->Release();
    return 0;
}

int UFXTraderApi::ReqOrderAction(CSecurityFtdcInputOrderActionField *pInputOrderAction, int nRequestID) {
    int iRet = 0, hSend = 0;
    ///获取版本为2类型的pack打包器
    IF2Packer *pPacker = NewPacker(2);
    if (!pPacker) {
        printf("取打包器失败!\r\n");
        return -1;
    }
    pPacker->AddRef();
    ///定义解包器
    //IF2UnPacker *pUnPacker = NULL;
    IBizMessage *lpBizMessage = NewBizMessage();
    lpBizMessage->AddRef();
    ///应答业务消息
    IBizMessage *lpBizMessageRecv = NULL;
    //功能号
    lpBizMessage->SetFunction(REQ_ORDER_ACTION);
    //请求类型
    lpBizMessage->SetPacketType(REQUEST_PACKET);
    lpBizMessage->SetSystemNo(_sysnode_id);
    lpBizMessage->SetSenderId(++_requestID);
    ///其他的应答信息
    LPRET_DATA pRetData = NULL;
    ///开始打包
    pPacker->BeginPack();
    ///加入字段名
    pPacker->AddField("op_branch_no", 'I', 5);//名字 类型 长度
    pPacker->AddField("op_entrust_way", 'C', 1);
    pPacker->AddField("op_station", 'S', 255);
    pPacker->AddField("branch_no", 'I', 5);
    pPacker->AddField("client_id", 'S', 18);//客户ID
    pPacker->AddField("fund_account", 'S', 18);//资金账号
    pPacker->AddField("password", 'S', 10);
    pPacker->AddField("password_type", 'C', 1);
    pPacker->AddField("user_token", 'S', 40);
    pPacker->AddField("batch_flag", 'S', 15);
    pPacker->AddField("exchange_type", 'S', 4);
    pPacker->AddField("entrust_no", 'S', 6);
    ///加入对应的字段值
    pPacker->AddInt(0);
    pPacker->AddChar(_entrustWay);
    pPacker->AddStr(_op_station);
    pPacker->AddInt(_branch_no);
    pPacker->AddStr(_client_id);
    pPacker->AddStr(_fund_account);
    pPacker->AddStr(_password);
    pPacker->AddChar('2');
    pPacker->AddStr(_user_token);
    pPacker->AddStr("0");
    pPacker->AddStr(pInputOrderAction->ExchangeID);
    pPacker->AddStr(pInputOrderAction->OrderRef); //SysID passed in OrderRef
//    pPacker->AddStr("0");

    pPacker->EndPack();
    lpBizMessage->SetContent(pPacker->GetPackBuf(), pPacker->GetPackLen());
    _lpConn->SendBizMsg(lpBizMessage, 1);
    pPacker->FreeMem(pPacker->GetPackBuf());
    pPacker->Release();
    lpBizMessage->Release();
    request2OrderAction[_requestID] = pInputOrderAction->OrderRef;
    return 0;
}


int UFXTraderApi::ReqUserLogout(CSecurityFtdcUserLogoutField *pUserLogout, int nRequestID) {
    return 0;
}

int UFXTraderApi::Join() {
    return 0;
}

void UFXTraderApi::Release() {

}

int UFXTraderApi::SubscribeOrderRsp() {
    int iRet = 0, hSend = 0;
    IF2UnPacker *pMCUnPacker = NULL;
    ///获取版本为2类型的pack打包器
    IF2Packer *pPacker = NewPacker(2);
    if (!pPacker) {
        printf("取打包器失败!\r\n");
        return -1;
    }
    pPacker->AddRef();
    ///定义解包器
    //IF2UnPacker *pUnPacker = NULL;
    IBizMessage *lpBizMessage = NewBizMessage();
    lpBizMessage->AddRef();
    ///应答业务消息
    IBizMessage *lpBizMessageRecv = NULL;
    //功能号
    lpBizMessage->SetFunction(MSGCENTER_FUNC_REG);
    lpBizMessage->SetIssueType(ISSUE_TYPE_ENTR_BACK);
    lpBizMessage->SetSequeceNo(13);
    //请求类型
    lpBizMessage->SetPacketType(REQUEST_PACKET);
    ///开始打包
    pPacker->BeginPack();
    ///加入字段名
    pPacker->AddField("branch_no", 'I', 5);
    pPacker->AddField("fund_account", 'S', 18);
    pPacker->AddField("issue_type", 'I', 8);
    ///加入对应的字段值
    pPacker->AddInt(_branch_no);
    pPacker->AddStr(_fund_account);
    pPacker->AddInt(ISSUE_TYPE_ENTR_BACK);
    ///结束打包
    pPacker->EndPack();
    lpBizMessage->SetKeyInfo(pPacker->GetPackBuf(), pPacker->GetPackLen());
    _lpConn->SendBizMsg(lpBizMessage, 1);
    pPacker->FreeMem(pPacker->GetPackBuf());
    pPacker->Release();
    lpBizMessage->Release();
    return 0;
}

int UFXTraderApi::SubscribeTradeRsp() {
    int iRet = 0, hSend = 0;
    IF2UnPacker *pMCUnPacker = NULL;

    ///获取版本为2类型的pack打包器
    IF2Packer *pPacker = NewPacker(2);
    if (!pPacker) {
        printf("取打包器失败!\r\n");
        return -1;
    }
    pPacker->AddRef();

    ///定义解包器
    //IF2UnPacker *pUnPacker = NULL;

    IBizMessage *lpBizMessage = NewBizMessage();

    lpBizMessage->AddRef();

    ///应答业务消息
    IBizMessage *lpBizMessageRecv = NULL;
    //功能号
    lpBizMessage->SetFunction(MSGCENTER_FUNC_REG);
    lpBizMessage->SetIssueType(ISSUE_TYPE_REALTIME_SECU);
    lpBizMessage->SetSenderId(_requestID++);
    //请求类型
    lpBizMessage->SetPacketType(REQUEST_PACKET);

    ///开始打包
    pPacker->BeginPack();

    ///加入字段名
    pPacker->AddField("branch_no", 'I', 5);
    pPacker->AddField("fund_account", 'S', 18);
    pPacker->AddField("op_branch_no", 'I', 5);
    pPacker->AddField("op_entrust_way", 'C', 1);
    pPacker->AddField("op_station", 'S', 255);
    pPacker->AddField("client_id", 'S', 18);
    pPacker->AddField("password", 'S', 10);
    pPacker->AddField("user_token", 'S', 40);
    pPacker->AddField("issue_type", 'I', 8);

    ///加入对应的字段值
    pPacker->AddInt(_branch_no);
    pPacker->AddStr(_fund_account);
    pPacker->AddInt(0);
    pPacker->AddChar(_entrustWay);
    pPacker->AddStr(_op_station);    //op_station
    pPacker->AddStr(_client_id);
    pPacker->AddStr(_password);
    pPacker->AddStr(_user_token);
    pPacker->AddInt(ISSUE_TYPE_REALTIME_SECU);
    ///结束打包
    pPacker->EndPack();
    lpBizMessage->SetKeyInfo(pPacker->GetPackBuf(), pPacker->GetPackLen());
    iRet = _lpConn->SendBizMsg(lpBizMessage, 1);
    std::cout << "SendBizMsg: " << iRet << std::endl;
    if (iRet < 0) {
        std::cout << _lpConn->GetErrorMsg(iRet) << std::endl;
    }
    pPacker->FreeMem(pPacker->GetPackBuf());
    pPacker->Release();
    lpBizMessage->Release();
    return iRet;
}

int UFXTraderApi::ReqQryTradingAccount(CSecurityFtdcQryTradingAccountField *pQryTradingAccount, int nRequestID) {
    int iRet = 0, hSend = 0;
    ///获取版本为2类型的pack打包器
    IF2Packer *pPacker = NewPacker(2);
    if (!pPacker) {
        printf("取打包器失败!\r\n");
        return -1;
    }
    pPacker->AddRef();
    ///定义解包器
    //IF2UnPacker *pUnPacker = NULL;
    IBizMessage *lpBizMessage = NewBizMessage();
    lpBizMessage->AddRef();
    ///应答业务消息
    IBizMessage *lpBizMessageRecv = NULL;
    //功能号
    lpBizMessage->SetFunction(QRY_MONEY);
    //请求类型
    lpBizMessage->SetPacketType(REQUEST_PACKET);
    lpBizMessage->SetSystemNo(_sysnode_id);
    ///其他的应答信息
    LPRET_DATA pRetData = NULL;
    ///开始打包
    pPacker->BeginPack();
    ///加入字段名
    pPacker->AddField("op_branch_no", 'I', 5);//名字 类型 长度
    pPacker->AddField("op_entrust_way", 'C', 1);
    pPacker->AddField("op_station", 'S', 255);
    pPacker->AddField("branch_no", 'I', 5);
    pPacker->AddField("client_id", 'S', 18);//客户ID
    pPacker->AddField("fund_account", 'S', 18);//资金账号
    pPacker->AddField("password", 'S', 10);
    pPacker->AddField("password_type", 'C', 1);
    pPacker->AddField("user_token", 'S', 40);
    ///加入对应的字段值
    pPacker->AddInt(0);
    pPacker->AddChar(_entrustWay);
    pPacker->AddStr(_op_station);
    pPacker->AddInt(_branch_no);
    pPacker->AddStr(_client_id);
    pPacker->AddStr(_fund_account);
    pPacker->AddStr(_password);
    pPacker->AddChar('2');
    pPacker->AddStr(_user_token);

    pPacker->EndPack();
    lpBizMessage->SetContent(pPacker->GetPackBuf(), pPacker->GetPackLen());
    _lpConn->SendBizMsg(lpBizMessage, 1);
    pPacker->FreeMem(pPacker->GetPackBuf());
    pPacker->Release();
    lpBizMessage->Release();
    return 0;
}

int UFXTraderApi::ReqQryInvestorPosition(CSecurityFtdcQryInvestorPositionField *pQryInvestorPosition, int nRequestID) {
    IBizMessage *lpBizMessage = NewBizMessage();
    lpBizMessage->AddRef();
    IBizMessage *lpBizMessageRecv = NULL;
    //功能号
    lpBizMessage->SetFunction(QRY_POSITION);
    //请求类型
    lpBizMessage->SetPacketType(REQUEST_PACKET);
    //设置系统号
    lpBizMessage->SetSystemNo(_sysnode_id);

    ///获取版本为2类型的pack打包器
    IF2Packer *pPacker = NewPacker(2);
    if (!pPacker) {
        printf("取打包器失败!\r\n");
        return -1;
    }
    pPacker->AddRef();

    ///开始打包
    pPacker->BeginPack();

    ///加入字段名
    pPacker->AddField("op_branch_no", 'I', 5);
    pPacker->AddField("op_entrust_way", 'C', 1);
    pPacker->AddField("op_station", 'S', 255);
    pPacker->AddField("branch_no", 'I', 5);
    pPacker->AddField("client_id", 'S', 18);
    pPacker->AddField("fund_account", 'S', 18);
    pPacker->AddField("password", 'S', 10);
    pPacker->AddField("password_type", 'C', 1);
    pPacker->AddField("user_token", 'S', 512);

    pPacker->AddField("exchange_type", 'S', 4);
    pPacker->AddField("stock_code", 'S', 11);
    pPacker->AddField("query_mode", 'C', 1);


    pPacker->AddField("position_str", 'S', 100);
    pPacker->AddField("request_num", 'N', 10);

    ///加入对应的字段值
    pPacker->AddInt(0);            //	op_branch_no
    pPacker->AddChar(_entrustWay);            //		op_entrust_way
    pPacker->AddStr(_op_station);        //op_station
    pPacker->AddInt(_branch_no);    //	branch_no
    pPacker->AddStr(_client_id);    //		client_id
    pPacker->AddStr(_fund_account);    //	fund_account
    pPacker->AddStr(_password);        //	password
    pPacker->AddChar('1');                //	password_type
    pPacker->AddStr(_user_token);    //user_token

    pPacker->AddStr("2");
    pPacker->AddStr("000605");
    pPacker->AddChar('0');


    pPacker->AddStr(" ");
    pPacker->AddInt(3);


    ///结束打包
    pPacker->EndPack();

    lpBizMessage->SetContent(pPacker->GetPackBuf(), pPacker->GetPackLen());
    _lpConn->SendBizMsg(lpBizMessage, 1);
    pPacker->FreeMem(pPacker->GetPackBuf());
    pPacker->Release();
    lpBizMessage->Release();
}
