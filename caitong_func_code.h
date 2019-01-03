//
// Created by ht on 18-12-4.
//

#ifndef UFXADAPTER_UFX_FUNC_CODE_H
#define UFXADAPTER_UFX_FUNC_CODE_H

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

#endif //UFXADAPTER_UFX_FUNC_CODE_H
