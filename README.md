crashstat
=========

whole sdk to generate crash dmp and view statistics of all dmps


Architecture
=========

1. [Data Flow](https://github.com/hufuman/crashstat/raw/master/docs/data_flow.png)


Client Part
=========

1. Crash Daemon
2. Crash Uploader

Server Part
=========

1. Crash Receiver
2. Parser Scheduler
3. Crash Parser

Build Part
=========
1. Source Indexer
2. Symbol Uploader

Web Part
=========

1. Web
2. Web Service
