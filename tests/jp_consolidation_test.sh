#!/bin/sh

RED="\033[1;31m"
GREEN="\033[1;32m"
WHITE="\033[0m"


./json_packer ./json-input/input-json.1.txt kv_pair_1.tlv key_index_1.tlv
./json_packer ./json-input/input-json.2.txt kv_pair_2.tlv key_index_2.tlv
./json_packer ./json-input/input-json.3.txt kv_pair_3.tlv key_index_3.tlv

./tlv_consolidator kv_pair_1.tlv key_index_1.tlv kv_pair_2.tlv key_index_2.tlv kv_pair_3.tlv key_index_3.tlv

./tlv_unpacker consolidated_kv_pair.tlv consolidated_key_index.tlv

if [ $? -eq 0 ]; then
		echo -e "${GREEN}SUCCESS${WHITE}:  $file"
	else
		echo -e "${RED}FAILED${WHITE} :  $file"
fi