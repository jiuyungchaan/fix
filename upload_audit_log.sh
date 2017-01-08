#!/bin/bash

home=/home/rongcan
yymmdd=`date "+%Y%m%d"`
yymmdd=20170106
session_id=ltd3qun
ecid=9670533
market_code=glbx
platform_code=prop
client_name=cashalgo
detail=cfiteam

audit_log_file=${yymmdd}_${ecid}_${market_code}_${platform_code}_${session_id}_${client_name}_${detail}_audittrail.globex.csv
audit_log_gzip=${yymmdd}_${ecid}_${market_code}_${platform_code}_${session_id}_${client_name}_${detail}_audittrail.csv.gzip
done_file=${yymmdd}_${ecid}_${market_code}_${platform_code}_${session_id}_${client_name}_${detail}_done.done
done_gzip=${yymmdd}_${ecid}_${market_code}_${platform_code}_${session_id}_${client_name}_${detail}_done.gzip
#echo $audit_log_file $done_file

echo "Download $audit_log_file..."
cd $home/cme/audit_trail
scp -P 44149 cfi@101.231.3.117:/home/cfi/test/$audit_log_file ${audit_log_file}.noheader
if [ -f ${audit_log_file}.noheader ]
then
  cat audit_header.csv ${audit_log_file}.noheader > $audit_log_file
  rm ${audit_log_file}.noheader
  echo "RECORD_COUNT=`wc -l $audit_log_file | cut -d \" \" -f1`" > $done_file
else
  cat audit_header.csv > $audit_log_file
  echo "RECORD_COUNT=0" > $done_file
fi
gzip -c $audit_log_file > $audit_log_gzip
gzip -c $done_file > $done_gzip

# ftp operations
echo "Uplaod $audit_log_gzip and $done_gzip to ftp..."
ftp -n<<!
open ftp.newedgegroup.com
user SDMA-CashAlgo Km1@2Laaw
binary
hash
cd /AuditLogFiles
lcd /home/rongcan/cme/audit_trail
prompt
put $audit_log_gzip
put $done_gzip
close
bye
!
