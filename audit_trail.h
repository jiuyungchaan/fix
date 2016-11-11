#ifndef __AUDIT_TRAIL_H__
#define __AUDIT_TRAIL_H__

#include <string>
#include <map>
#include <fstream>

class AuditElements {
 public:
  AuditElements();
  ~AuditElements();

 private:

};

class AuditLog {
 public:
  AuditLog();
  ~AuditLog();

  void ClearElement(std::string key);

  int WriteElement(std::string key, std::string value);
  int WriteElement(std::string key, const char *value);
  int WriteElement(std::string key, int value);
  int WriteElement(std::string key, double value);
  int WriteElement(std::string key, char value);

  std::string ToString();

 private:
  std::map<std::string, std::string> elements;
};

class AuditTrail {
 public:
  AuditTrail();
  ~AuditTrail();

  int Init(const char *log_file_name);
  void WriteLog(AuditLog& log);

 private:
  std::fstream log_file_;
};

#endif  // __AUDIT_TRAIL_H__
