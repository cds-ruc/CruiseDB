# CruiseDB: An SLA-Oriented LSM-Tree Key-Value Store for High-end Cloud Data Service

This is the prototype of CruiseDB for the ICDE 2021 submission.

CruiseDB is built as an extension of RocksDB https://rocksdb.org/

Important files for CruiseDB implementation:
db_impl.cc
db_impl_compaction_flush.cc
db_impl_write.cc
io_controller.h
io_controller.cc
io_posix.cc
token_bucket.h
token_bucket.cc
rate_estimater.h
rate_estimater.cc

Some parameters need to be set according to system ability in rate_estimater.h and rate_estimater.cc

This is a work of Key Laboratory of DEKE, MOE, China form School of Information, Renmin University of China