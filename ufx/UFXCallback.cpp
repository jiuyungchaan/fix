//
// Created by ht on 18-12-18.
//

#include "UFXCallback.h"

void Callback::OnReceivedBizMsg(CConnectionInterface *lpConnection, int hSend, IBizMessage *lpMsg) {
    if (lpMsg != NULL) {
        int nRequestID = lpMsg->GetSenderId();
        //成功,应用程序不能释放lpBizMessageRecv消息
        if (lpMsg->GetReturnCode() == 0) {
            int iLen = 0;
            const void *lpBuffer = lpMsg->GetContent(iLen);
            IF2UnPacker *lpUnPacker = NewUnPacker((void *) lpBuffer, iLen);
            int iLen_key = 0;
            const void *lpBuffer_key = lpMsg->GetKeyInfo(iLen_key);
            IF2UnPacker *lpUnPacker_key = NewUnPacker((void *) lpBuffer_key, iLen_key);
            printf("function %d\n", lpMsg->GetFunction());
            switch (lpMsg->GetFunction()) {
                case REQ_CLIENT_LOGIN: {
                    if (lpUnPacker) {
                        lpUnPacker->AddRef();
                        OnResponse_LOGIN(lpUnPacker, nRequestID);
                        lpUnPacker->Release();
                    }
                    break;
                }
                case REQ_ORDER_INSERT: {
                    if (lpUnPacker) {
                        lpUnPacker->AddRef();
                        OnResponse_ORDERINSERT(lpUnPacker, nRequestID);
                        lpUnPacker->Release();
                    }
                    break;
                }
                case REQ_ORDER_ACTION: {
                    if (lpUnPacker) {
                        lpUnPacker->AddRef();
                        OnResponse_ORDERACTION(lpUnPacker, nRequestID);
                        lpUnPacker->Release();
                    }
                    break;
                }
                case MSGCENTER_FUNC_HEART: {
                    if (lpMsg->GetPacketType() == REQUEST_PACKET) {
                        lpMsg->ChangeReq2AnsMessage();
                        if (lpConnection != NULL)
                            lpConnection->SendBizMsg(lpMsg, 1);
#ifndef NDEBUG
                        cout << "heartBeat.." << endl;
#endif
                        _spi->OnHeartBeatWarning(0);
                    }
                    break;
                }
                case MSGCENTER_FUNC_REG: {
                    int iLen_ley = 0;
                    const void *lpBuffer_key = lpMsg->GetKeyInfo(iLen_ley);
                    int type = lpMsg->GetIssueType();
#ifndef NDEBUG
                    printf("订阅类型：%d\n", type);
#endif
                    IF2UnPacker *lpUnPacker_key = NewUnPacker((void *) lpBuffer_key, iLen_ley);
                    if (lpUnPacker_key) {
                        lpUnPacker_key->AddRef();
                        if (lpUnPacker_key->GetInt("error_no") != 0) {
                            puts(lpUnPacker_key->GetStr("error_info"));
                            lpUnPacker_key->Release();
                        } else {
#ifndef NDEBUG
                            puts("订阅成功...");
                            ShowPacket(lpUnPacker_key);
#endif
                            lpUnPacker_key->Release();
                        }
                    }
                    break;
                }
                case MSGCENTER_FUNC_SENDED: {
                    int iLen_ley = 0;
                    const void *lpBuffer_key = lpMsg->GetKeyInfo(iLen_ley);
                    IF2UnPacker *lpUnPacker_key = NewUnPacker((void *) lpBuffer_key, iLen_ley);

                    if (lpUnPacker_key) {
                        lpUnPacker_key->AddRef();
                        if (lpMsg->GetIssueType() == ISSUE_TYPE_REALTIME_SECU) {
                            printf("Trade！\n");
//                            ShowPacket(lpUnPacker_key);
                            lpUnPacker_key->Release();
                            if (lpUnPacker) {
                                lpUnPacker->AddRef();
                                char real_type = lpUnPacker->GetChar("real_type");
                                char real_status = lpUnPacker->GetChar("real_status");
                                if (real_type == '0' && real_status == '0') { // rtn trade
                                    OnRtn_TRADE(lpUnPacker, nRequestID);
                                } else if (real_type == '2' && real_status == '2') { // 撤单失败
                                    OnResponse_ORDERACTION(lpUnPacker, nRequestID);
                                } else { // rtn order, 废单，撤单，
                                    OnRtn_ORDER(lpUnPacker, nRequestID);
                                }
                                lpUnPacker->Release();
                            }
                        }
                        if (lpMsg->GetIssueType() == ISSUE_TYPE_ENTR_BACK) {
                            printf("Order Insert！\n");
                            OnRtn_ORDER(lpUnPacker, nRequestID);
                            lpUnPacker_key->Release();
                        }
                    }
                    break;
                }
                case QRY_MONEY: {
#ifndef NDEBUG
                    ShowPacket(lpUnPacker);
#endif
                    OnRsp_QRY_TRADING_ACCOUNT(lpUnPacker, nRequestID);
                    break;
                }
                case QRY_POSITION: {
#ifndef NDEBUG
                    ShowPacket(lpUnPacker);
#endif
                    OnRsp_QRY_POSITION(lpUnPacker, nRequestID);
                    break;
                }
                default:
                    break;
            }
        } else {
            int iLen = 0;
            const void *lpBuffer = lpMsg->GetContent(iLen);
            IF2UnPacker *lpUnPacker = NewUnPacker((void *) lpBuffer, iLen);
            if (lpUnPacker != NULL) {
                lpUnPacker->AddRef();//添加释放内存引用
#ifndef NDEBUG
                ShowPacket(lpUnPacker);
#endif
                lpUnPacker->Release();
            } else {
                char buff[128];
                const char *msg = lpMsg->GetErrorInfo();
                g2u(msg, strlen(msg), buff, sizeof(buff));
                printf("业务包是空包，错误代码：%d，错误信息:%s\n", lpMsg->GetErrorNo(), buff);
            }
        }
    }
}

void Callback::OnResponse_LOGIN(IF2UnPacker *lpUnPacker, int nRequestID) {
    CSecurityFtdcRspUserLoginField field;
    CSecurityFtdcRspInfoField rspInfo{0};
    sprintf(field.LoginTime, "%d", lpUnPacker->GetInt("init_date"));
    strcpy(field.UserID, lpUnPacker->GetStr("account_content"));
    field.SessionID = lpUnPacker->GetInt("session_no");
    strcpy(field.BrokerID, lpUnPacker->GetStr("company_name"));
    _api->LoginSetup(
            lpUnPacker->GetInt("branch_no"),
            lpUnPacker->GetInt("sysnode_id"),
            lpUnPacker->GetStr("client_id"),
            lpUnPacker->GetStr("user_token"),
            lpUnPacker->GetStr("fund_account")
    );

    _spi->OnRspUserLogin(&field, &rspInfo, nRequestID, true);
}

void Callback::OnResponse_ORDERINSERT(IF2UnPacker *lpUnPacker, int nRequestID) {
    CSecurityFtdcInputOrderField inputOrderField;
    CSecurityFtdcRspInfoField rspInfo{0};
//    ShowPacket(lpUnPacker);
    sprintf(inputOrderField.OrderRef, "%d", lpUnPacker->GetInt("batch_no"));
    _spi->OnRspOrderInsert(&inputOrderField, &rspInfo, nRequestID, true);
}

void Callback::OnResponse_ORDERACTION(IF2UnPacker *lpUnPacker, int nRequestID) {
    cout << "order action:" << endl;
#ifndef NDEBUG
    ShowPacket(lpUnPacker);
#endif
    CSecurityFtdcOrderActionField orderActionField;
    CSecurityFtdcRspInfoField rspInfo{0};
//    orderActionField.Or
//                    orderActionField.OrderRef
    _spi->OnRspOrderAction(&orderActionField, &rspInfo, nRequestID, true);
}

void Callback::OnRtn_ORDER(IF2UnPacker *lpUnPacker, int nRequestID) {
#ifndef NDEBUG
    ShowPacket(lpUnPacker);
#endif
    CSecurityFtdcOrderField orderField;
    orderField.SessionID = lpUnPacker->GetInt("session_no");
    sprintf(orderField.OrderSysID, "%d", lpUnPacker->GetInt("order_id"));
    sprintf(orderField.OrderRef, "%d", lpUnPacker->GetInt("batch_no"));
    if (lpUnPacker->GetInt("issue_type") == ISSUE_TYPE_REALTIME_SECU) { //rtn trade, 撤单 or 废单
        orderField.OrderStatus = SECURITY_FTDC_OST_Canceled;
        _spi->OnRtnOrder(&orderField);

    } else { // rtn order insert
        char entrust_type = lpUnPacker->GetChar("entrust_type");
        if (entrust_type == '0') { // 买卖委托回报
            orderField.OrderStatus = SECURITY_FTDC_OST_NoTradeQueueing;
            sprintf(orderField.OrderRef, "%d", lpUnPacker->GetInt("batch_no"));
            _spi->OnRtnOrder(&orderField);

        } else if (entrust_type == '2') { // 撤单委托回报
            // no response
        }
    }
}

void Callback::OnRtn_TRADE(IF2UnPacker *lpUnPacker, int nRequestID) {
#ifndef NDEBUG
    ShowPacket(lpUnPacker);
#endif

    CSecurityFtdcTradeField tradeField;
    strcpy(tradeField.OrderSysID, lpUnPacker->GetStr("order_id"));
    sprintf(tradeField.OrderRef, "%d", lpUnPacker->GetInt("batch_no"));
    strcpy(tradeField.InstrumentID, lpUnPacker->GetStr("stock_code"));
    tradeField.Direction = (lpUnPacker->GetChar("entrust_bs") == '1') ? SECURITY_FTDC_D_Buy : SECURITY_FTDC_D_Sell;
    tradeField.Volume = lpUnPacker->GetDouble("business_amount");
    sprintf(tradeField.Price, "%lf", lpUnPacker->GetDouble("business_price"));
    _spi->OnRtnTrade(&tradeField);
}

void Callback::OnRsp_QRY_TRADING_ACCOUNT(IF2UnPacker *lpUnPacker, int nRequestID) {
    CSecurityFtdcTradingAccountField result;
    result.Balance = lpUnPacker->GetDouble("current_balance");
    // todo other info
    CSecurityFtdcRspInfoField rspInfo{0};
    _spi->OnRspQryTradingAccount(&result, &rspInfo, nRequestID, true);
}

void Callback::OnRsp_QRY_POSITION(IF2UnPacker *lpUnPacker, int nRequestID) {
    CSecurityFtdcQryInvestorPositionField result;
    CSecurityFtdcRspInfoField rspInfo{0};
    _spi->OnRspQryInvestorPosition(&result, &rspInfo, nRequestID, true);
}
