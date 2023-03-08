

 Json Packer


 This project compiles as other cmake projects:

 mkdir build/
 cd build/
 cmake ..
 make

 Once the build completes, the following executables are available:

 json_packer
 tlv_unpacker
 tlv_consolidator

 json_packer expects an input JSON filename and optionally two filenames for the output set key-value pair and key index TLV encoded files

 tlv_unpacker expects two files from the same set for the input key-value pair and the key index TLV encoded files

 tlv_consolidator expects an even list of filenames (two filename for every file set) of key-value pair and key index TLV files (in that order).
 The output of tlv_consolidator will be a single set of files:

 consolidated_kv_pair.tlv
 consolidated_key_index.tlv

 this will contain all the aggregated records of all the input file sets.

 === tests ===

  two set of tests can be run:

 ./json_packer_tests (right now only key-value pair encoding to and from buffer tests)

 ./jp_consolidation_test.sh exercises consolidation from 3 json record files, and finally unpacks the content of the consolidated file set



 Author:
 Charles J. Quarra


 charlesjquarra@gmail.com