
#ifndef __X1TRADER_H__
#define __X1TRADER_H__

#include "X1FtdcTraderApi.h"

#include "../order.h"

#include <string>

class X1Trader : public x1ftdcapi::CX1FtdcTraderSpi {
 public:
  X1Trader();
  virtual ~X1Trader();

  void Logon(char *pszFrontAddr);

  // spi interfaces
  virtual void OnFrontConnected();
  virtual void OnFrontDisconnected();
  virtual void OnRspUserLogin(struct CX1FtdcRspUserLoginField* pUserLoginInfoRtn, 
        struct CX1FtdcRspErrorField* pErrorInfo);
  virtual void OnRspUserLogout(struct CX1FtdcRspUserLogoutInfoField* pUserLogoutInfoRtn,
        struct CX1FtdcRspErrorField* pErrorInfo);
  virtual void OnRspInsertOrder(struct CX1FtdcRspOperOrderField* pOrderRtn,
        struct CX1FtdcRspErrorField* pErrorInfo);
  virtual void OnRspCancelOrder(struct CX1FtdcRspOperOrderField* pOrderCanceledRtn,
        struct CX1FtdcRspErrorField* pErrorInfo);
  virtual void OnRtnErrorMsg(struct CX1FtdcRspErrorField* pErrorInfo);
  virtual void OnRtnMatchedInfo(struct CX1FtdcRspPriMatchInfoField* pRtnMatchData);
  virtual void OnRtnOrder(struct CX1FtdcRspPriOrderField* pRtnOrderData);
  virtual void OnRtnCancelOrder(struct CX1FtdcRspPriCancelOrderField* pCancelOrderData);
  // virtual void OnRspCancelAllOrder(struct CX1FtdcCancelAllOrderRspField *pRspCancelAllOrderData,
  //       struct CX1FtdcRspErrorField * pErrorInfo);
  virtual void OnRspQryPosition(struct CX1FtdcRspPositionField* pPositionInfoRtn,
        struct CX1FtdcRspErrorField* pErrorInfo, bool bIsLast);

  void ReqInsertOrder(Order *order);
  void ReqCancelOrder(Order *order);
  void ReqCancelOrder(int order_id, long x1order_id, const char *instrument_id);
  void ReqCancelAllOrder();
  void ReqQryPosition();
  void Logout();

  std::string user_id_;
  std::string password_;

 private:
  x1ftdcapi::CX1FtdcTraderApi *api_;
  OrderPool order_pool_;
  int request_id_;
  bool logon_;
};

#endif  // __X1TRADER_H__
