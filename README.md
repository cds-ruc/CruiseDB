# CruiseDB: An SLA-Oriented LSM-Tree Key-Value Store for High-end Cloud Data Service

This is the prototype of CruiseDB for the ICDE 2021 submission.

CruiseDB is built as an extension of RocksDB https://rocksdb.org/

Important files for CruiseDB implementation:<br>
db_impl.cc<br>
db_impl_compaction_flush.cc<br>
db_impl_write.cc<br>
io_controller.h<br>
io_controller.cc<br>
io_posix.cc<br>
token_bucket.h<br>
token_bucket.cc<br>
rate_estimater.h<br>
rate_estimater.cc

Some parameters need to be set according to system ability in rate_estimater.h and rate_estimater.cc

This is a work of Key Laboratory of DEKE, MOE, China from School of Information, Renmin University of China
