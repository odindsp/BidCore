CC = g++

OBJECTS=libtanx.so libamax.so libinmobi.so libletv.so libiwifi.so libtoutiao.so libsohu.so libbaidu.so libgoyoo.so libzplay.so \
	libiqiyi.so libiflytek.so libgdt.so libadview.so libgoogle.so liblefee.so libautohome.so libbaiduvideo.so libmomo.so liblingji.so 

all:
	$(CC) tanx/DecodePrice.cpp -fPIC -shared -o libtanx.so -lcrypto -O2
	${CC} amax/decrypter.cc -fPIC -shared -o libamax.so -lcrypto -O2
	${CC} inmobi/decode.cpp -fPIC -shared -o libinmobi.so -lcrypto -O2
	${CC} letv/decode.cpp ../baselib/base64.c ../baselib/algo_aes.cpp -fPIC -shared -o libletv.so -lcrypto -O2
	${CC} iwifi/decode.cpp -fPIC -shared -o libiwifi.so -lcrypto -O2
	${CC} toutiao/DecodePrice.cpp -fPIC -shared -o libtoutiao.so -lcrypto -O2
	${CC} sohu/priceDecrypt.cpp -fPIC -shared -o libsohu.so -lcrypto -O2
	${CC} baidu/decode.cpp -fPIC -shared -o libbaidu.so -lcrypto -lssl -O2
	${CC} goyoo/decode.cpp -fPIC -shared -o libgoyoo.so -lcrypto -lssl -O2
	gcc	  ./zplay/decode.c ./zplay/hmac_sha1.c ./zplay/hex.c ./zplay/base64.c -fPIC -shared -o libzplay.so --std=c99 -O2
	${CC} ./iqiyi/decode.cpp ./iqiyi/settlement.pb.cc -fPIC -shared -o libiqiyi.so -lssl -lcrypto -lprotobuf -O2
	${CC} ./iflytek/decode.cpp ./iflytek/base64.c -fPIC -shared -o libiflytek.so -lssl -lcrypto -O2
	${CC} ./gdt/decode.cpp ./gdt/base64.c -fPIC -shared -o libgdt.so -lssl -lcrypto -O2
	${CC} ./adview/decrypter.cc -fPIC -shared -o libadview.so -lcrypto -O2
	${CC} ./google/decrypter.cc -fPIC -shared -o libgoogle.so -lcrypto -O2
	${CC} ./lefee/decrypter.cc -fPIC -shared -o liblefee.so -lcrypto -O2
	gcc   ./autohome/adx_encrypt.c  -o libautohome.so -fPIC -shared -lcrypto -lssl -O2 -std=c99
	${CC} ./baiduvideos/decode.cpp -fPIC -shared -o libbaiduvideo.so -lcrypto -O2
	${CC} ./momo/decode.cpp ./momo/base64.c -fPIC -shared -o libmomo.so -lssl -lcrypto -O2
	${CC} ./lingji/mz_decode.cpp ./lingji/base64.cpp ./lingji/openssl_aes.cpp -fPIC -shared -o liblingji.so -lssl -lcrypto -O2
install:
	mv *.so /usr/local/lib64/dspads_odin/

.PHONY: clean
clean:
	-rm $(OBJECTS)

#end of file

