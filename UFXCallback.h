//
// Created by ht on 18-12-18.
//

#ifndef UFXADAPTER_CALLBACK_H
#define UFXADAPTER_CALLBACK_H


#include <t2sdk_interface.h>
#include "UFXTraderApi.h"

class Callback : public CCallbackInterface {
public:
    explicit Callback(UFXTraderSpi *spi, UFXTraderApi *api) : _spi(spi), _api(api) {
    };

    ~Callback() {};

    // 因为CCallbackInterface的最终纯虚基类是IKnown，所以需要实现一下这3个方法
    unsigned long  FUNCTION_CALL_MODE QueryInterface(const char *iid, IKnown **ppv) { return 0; };

    unsigned long  FUNCTION_CALL_MODE AddRef() { return 0; };

    unsigned long  FUNCTION_CALL_MODE Release() { return 0; };

// 各种事件发生时的回调方法，实际使用时可以根据需要来选择实现，对于不需要的事件回调方法，可直接return
    // Reserved?为保留方法，为以后扩展做准备，实现时可直接return或return 0。
    void FUNCTION_CALL_MODE OnConnect(CConnectionInterface *lpConnection) override {

    }

    void FUNCTION_CALL_MODE OnSafeConnect(CConnectionInterface *lpConnection) override {
//        _spi->OnFrontConnected();//todo
        std::cout << "safe connect" << std::endl;
    }

    void FUNCTION_CALL_MODE OnRegister(CConnectionInterface *lpConnection) override {
        //todo
        std::cout << "on register" << std::endl;
        _spi->OnFrontConnected();

    }

    void FUNCTION_CALL_MODE OnClose(CConnectionInterface *lpConnection) override {
        std::cout << "on close" << std::endl;
        _spi->OnFrontDisconnected(0);
    }

    void FUNCTION_CALL_MODE
    OnSent(CConnectionInterface *lpConnection, int hSend, void *reserved1, void *reserved2, int nQueuingData) {};

    void FUNCTION_CALL_MODE Reserved1(void *a, void *b, void *c, void *d) override {};

    void FUNCTION_CALL_MODE Reserved2(void *a, void *b, void *c, void *d) override {};

    int  FUNCTION_CALL_MODE Reserved3() override { return 0; };

    void FUNCTION_CALL_MODE Reserved4() override {};

    void FUNCTION_CALL_MODE Reserved5() override {};

    void FUNCTION_CALL_MODE Reserved6() override {};

    void FUNCTION_CALL_MODE Reserved7() override {};

    // 收到SendBiz异步发送的请求的应答
    void FUNCTION_CALL_MODE
    OnReceivedBiz(CConnectionInterface *lpConnection, int hSend, const void *lpUnPackerOrStr, int nResult) override {
        std::cout << "received biz" << std::endl;
    };

    // 收到SendBiz异步发送的请求的应答
    void FUNCTION_CALL_MODE
    OnReceivedBizEx(CConnectionInterface *lpConnection, int hSend, LPRET_DATA lpRetData, const void *lpUnpackerOrStr,
                    int nResult) override {
        std::cout << "received biz ex" << std::endl;
    };

// 收到发送时指定了ReplyCallback选项的请求的应答或者是没有对应请求的数据
    void FUNCTION_CALL_MODE
    OnReceivedBizMsg(CConnectionInterface *lpConnection, int hSend, IBizMessage *lpMsg) override;


private:
    UFXTraderSpi *_spi;
    UFXTraderApi *_api;

    void OnResponse_LOGIN(IF2UnPacker *lpUnPacker, int nRequestID);

    void OnResponse_ORDERINSERT(IF2UnPacker *lpUnPacker, int nRequestID);

    void OnResponse_ORDERACTION(IF2UnPacker *lpUnPacker, int nRequestID);

    void OnRtn_ORDER(IF2UnPacker *lpUnPacker, int nRequestID);

    void OnRtn_TRADE(IF2UnPacker *lpUnPacker, int nRequestID);

    void OnRsp_QRY_TRADING_ACCOUNT(IF2UnPacker *lpUnPacker, int nRequestID);

    void OnRsp_QRY_POSITION(IF2UnPacker *lpUnPacker, int nRequestID);
};


#endif //UFXADAPTER_CALLBACK_H
