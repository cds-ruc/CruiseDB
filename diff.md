This file is to show the main differences we've made on RocksDB to implement CruiseDB.

#### The files we've added:
* db/io_controller.h
* db/io_controller.cc
* db/token_bucket.h
* db/token_bucket.cc
* db/rate_estimater.h
* db/rate_estimater.cc

#### The files we've modified:
* db/column_family.h
  * line 268 ~ 606
* db/column_family.cc
  * line 486 ~ 589
  * line 592 ~ 647
* db/compaction/compaction_job.cc
  * line 1463 ~ 1564
* db/db_impl/db_impl.cc
  * line 1530 ~ 1715
  * line 2575 ~ 2580
* db/db_impl/db_impl_compaction_flush.cc
  * line 2046 ~ 2053
  * line 2503 ~ 2563
* db/db_impl/db_impl_write.cc
  * line 1808 ~ 1850
* env/io_posix.cc
  * line 121 ~ 146
  * line 1178 ~ 1193

There are also some other minor changes we've made not listed.