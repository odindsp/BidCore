#ifndef		ERRORCODE_H_
#define		ERRORCODE_H_


#define		E_SUCCESS										0x0     //成功
#define		E_UNKNOWN										0xE0000000//未知问题，统一未知错误
#define		E_NO_AD_DATA									0xE0000001//程序没有找到广告信息

///////////////////////////
// *A*解析http 0xE0010000 - 0xE001FFFF
#define		__START_HTTP_CHECK								0xE0010000
#define		E_BAD_REQUEST									0xE0010001//[流量]广告请求内容解析失败，或者缺少某些关键字段
#define		E_REQUEST_HTTP_METHOD							0xE0010002//[流量] HTTP方法不对，有的需要是POST
#define		E_REQUEST_HTTP_CONTENT_TYPE						0xE0010003//[流量] HTTP CONTENT_TYPE 不匹配
#define		E_REQUEST_HTTP_CONTENT_LEN						0xE0010004//[流量] HTTP CONTENT_LEN 为0
#define		E_REQUEST_HTTP_BODY								0xE0010005//[流量] HTTP BODY 出错
#define		E_REQUEST_HTTP_REMOTE_ADDR						0xE0010006//[流量] HTTP REMOTE_ADDR没找到
#define		E_REQUEST_HTTP_OPENRTB_VERSION					0xE0010007//[流量] HTTP OPENRTB_VERSION没找到
#define		__END_HTTP_CHECK								0xE0010100

// *B* 请求基本信息 0xE0020001 - 0xE002FFFF
// 1 适配器 0xE0020001 - 0xE0020100
#define		__START_ADAPTER									0xE0020000
#define		__START_ADAPTER_BASE							0xE0020000//**基本信息
#define		E_REQUEST_IS_PING								0xE0020010//[流量] 请求是PING，可以当作是个测试类请求
#define		__START_ADAPTER_IMP								0xE0020100//**展现位信息
#define		__START_ADAPTER_DEVICE							0xE0020200//**device信息
#define		E_REQUEST_DEVICE								0xE0020201//[流量] 请求中的设备信息有问题
#define		__START_ADAPTER_APP								0xE0020300//**app信息
#define		__START_ADAPTER_USER							0xE0020400//**user信息
#define		__END_ADAPTER									0xE0021FFF


// 2 通用检查 0xE0022000 - 0xE002FFFF
#define		__START_CHECK									0xE0022000
#define		__START_CHECK_BASE								0xE0022000//**基本信息
#define		E_CHECK_BID_LEN									0xE0022001//[流量] 请求bid有问题
#define		E_CHECK_BID_TYPE								0xE0022002//[流量] 无效的竞价类型（目前只有rtb和pmp两种）
#define		__START_CHECK_IMP								0xE0022100//**展现位信息
#define		E_CHECK_PROCESS_CB								0xE0022101//[流量] 各个adx回调函数，一般是检查货币单位
#define		E_CHECK_IMP_SIZE_ZERO							0xE0022102//[流量] 请求里没有找到广告位
#define		E_CHECK_INSTL_SIZE_NOT_FIND						0xE0022110//[流量] banner 在adx下没有找到支持的该尺寸
#define		E_CHECK_NATIVE_LAYOUT							0xE0022120//[流量] 原生广告 布局样式不是信息流
#define		E_CHECK_NATIVE_PLCMTCNT							0xE0022121//[流量] 原生广告 数量小于1个
#define		E_CHECK_NATIVE_ASSETCNT							0xE0022122//[流量] 原生广告 元素列表太少（至少是title and image）
#define		E_CHECK_NATIVE_ASSET_TYPE						0xE0022130//[流量] 原生广告 ASSET类型不支持
#define		E_CHECK_NATIVE_ASSET_TITLE_LENGTH				0xE0022140//[流量] 原生广告 标题长度不合法
#define		E_CHECK_NATIVE_ASSET_IMAGE_SIZE					0xE0022150//[流量] 原生广告 image广告元素的尺寸宽高有0值
#define		E_CHECK_NATIVE_ASSET_IMAGE_TYPE					0xE0022151//[流量] 原生广告 图片类型不合法
#define		E_CHECK_NATIVE_ASSET_DATA_TYPE					0xE0022160//[流量] 原生广告 data元素的类型不符合（描述，rating，ctatext）
#define		E_CHECK_NATIVE_ASSET_DATA_LENGTH				0xE0022161//[流量] 原生广告 描述长度不符合
#define		__START_CHECK_DEVICE							0xE0022200//**device信息
#define		E_CHECK_DEVICE_OS								0xE0022201//[流量] 请求os信息不是ios和android
#define		E_CHECK_DEVICE_IP								0xE0022202//[流量] ip地址不在国内
#define		E_CHECK_DEVICE_TYPE								0xE0022203//[流量] 设备类型不被支持（目前只支持手机，平板和电视，个别adx只支持一种）
#define		E_CHECK_DEVICE_IP_BLACKLIST						0xE0022204//[流量] IP黑明单
#define		E_CHECK_DEVICE_ID_ALLBL							0xE002220F//[流量] 设备id在总黑名单中
#define		E_CHECK_DEVICE_ID_DID_UNKNOWN					0xE0022210//[流量] did类型不符合
#define		E_CHECK_DEVICE_ID_DID_IMEI						0xE0022211//[流量] imei不合法
#define		E_CHECK_DEVICE_ID_DID_MAC						0xE0022212//[流量] mac不合法
#define		E_CHECK_DEVICE_ID_DID_SHA1						0xE0022213//[流量] did的sha1值不合法
#define		E_CHECK_DEVICE_ID_DID_MD5						0xE0022214//[流量] did的md5值不合法
#define		E_CHECK_DEVICE_ID_DID_OTHER						0xE0022215//[流量] did的other类型长度为0
#define		E_CHECK_DEVICE_ID_DPID_UNKNOWN					0xE0022220//[流量] dpid类型不符合
#define		E_CHECK_DEVICE_ID_DPID_ANDROIDID				0xE0022221//[流量] android不合法
#define		E_CHECK_DEVICE_ID_DPID_IDFA						0xE0022222//[流量] idfa不合法
#define		E_CHECK_DEVICE_ID_DPID_SHA1						0xE0022223//[流量] dpid的sha1不合法
#define		E_CHECK_DEVICE_ID_DPID_MD5						0xE0022224//[流量] dpid的md5不合法
#define		E_CHECK_DEVICE_ID_DPID_OTHER					0xE0022225//[流量] dpid的other类型长度为0
#define		E_CHECK_DEVICE_MAKE_NIL							0xE0022240//[流量] 设备make字段为空
#define		E_CHECK_DEVICE_MAKE_OS							0xE0022241//[流量] 设备make与os不一致
#define		E_CHECK_TARGET_DEVICE							0xE0022250//[流量] 设备定向设置为：全部不支持
#define		E_CHECK_TARGET_DEVICE_REGIONCODE				0xE0022251//[流量] 地域定向失败
#define		E_CHECK_TARGET_DEVICE_CONNECTIONTYPE			0xE0022252//[流量] 网络连接类型定向失败
#define		E_CHECK_TARGET_DEVICE_OS						0xE0022253//[流量] 操作系统定向失败
#define		E_CHECK_TARGET_DEVICE_CARRIER					0xE0022254//[流量] 运营商定向失败
#define		E_CHECK_TARGET_DEVICE_DEVICETYPE				0xE0022255//[流量] 设别类型定向失败
#define		E_CHECK_TARGET_DEVICE_MAKE						0xE0022256//[流量] 制造商定向失败
#define		__START_CHECK_APP								0xE0022300//**app信息
#define		E_CHECK_TARGET_APP_ID							0xE0022310//[流量] app id定向过滤失败
#define		E_CHECK_TARGET_APP_CAT							0xE0022320//[流量] app cat定向过滤失败
#define		__START_CHECK_USER								0xE0022400//**user信息
#define		__END_CHECK										0xE002FFFF

// *C* 匹配 0xE0030000 - 0xE003FFFF
#define		__START_FILTER									0xE0030000
#define		__START_FILTER_FRC								0xE0030000//**频次
#define		E_FRC_CAMPAIGN									0xE0030001//[活动] 用户频次达到上限
#define		E_FRC_POLICY									0xE0030002//[策略] 用户频次达到上限
#define		E_FRC_CREATIVE									0xE0030003//[创意] 用户频次达到上限
#define		__START_FILTER_PAUSE							0xE0031000//**投放目标控制
#define		E_CAMPAIGN_PAUSE_KPI							0xE003100B//[活动] KPI达到上限
#define		E_CAMPAIGN_PAUSE_BUGET							0xE003100C//[活动] 预算达到上限
#define		E_POLICY_PAUSE_KPI								0xE0031015//[策略] KPI达到上限
#define		E_POLICY_PAUSE_BUGET							0xE0031016//[策略] 预算达到上限
#define		E_POLICY_PAUSE_UNIFORM							0xE0031017//[策略] 匀速投放达到上限
#define		E_POLICY_PAUSE_TIMESLOT							0xE0031018//[策略] 不在投放时段
#define		__START_FILTER_POLICY							0xE0032000//**策略 POLICY
#define		E_POLICY_CAT_IN_BL								0xE0032010//[策略] 广告主行业类型在流量的cat黑名单里
#define		E_POLICY_DOMAIN_IN_BADV							0xE0032020//[策略] adomain在流量的adv黑名单里
#define		E_POLICY_EXT									0xE0032030//[策略] ext过滤失败，一般为检查流量的advid和广告组的advid是否匹配
#define		E_POLICY_AT										0xE0032040//[策略] 投放模式不匹配（PMP RTB）
#define		E_POLICY_PMP_DEALID								0xE0032050//[策略] PMP类投放策略的dealid与流量给定的不符合
#define		E_POLICY_NOT_WL									0xE0032060//[策略] 设备ID不在投放策略设定的人群包白名单里
#define		E_POLICY_IN_BL									0xE0032070//[策略] 设备ID在投放策略设定的人群包黑名单里
#define		E_POLICY_NEED_DEVICEID							0xE0032080//[策略] 需要匹配流量里的设备ID，但流量没有给出设备ID
#define		E_POLICY_TARGET_APP_ID							0xE0032101//[策略] app id定向过滤失败
#define		E_POLICY_TARGET_APP_CAT							0xE0032102//[策略] app cat定向过滤失败
#define		E_POLICY_TARGET_DEVICE							0xE0032111//[策略] 设备定向设置为全部不支持
#define		E_POLICY_TARGET_DEVICE_REGIONCODE				0xE0032112//[策略] 地域定向失败
#define		E_POLICY_TARGET_DEVICE_CONNECTIONTYPE			0xE0032113//[策略] 网络连接类型定向失败
#define		E_POLICY_TARGET_DEVICE_OS						0xE0032114//[策略] 操作系统定向失败
#define		E_POLICY_TARGET_DEVICE_CARRIER					0xE0032115//[策略] 运营商定向失败
#define		E_POLICY_TARGET_DEVICE_DEVICETYPE				0xE0032116//[策略] 设别类型定向失败
#define		E_POLICY_TARGET_DEVICE_MAKE						0xE0032117//[策略] 制造商定向失败
#define		E_POLICY_TARGET_SCENE							0xE0032120//[策略] 场景定向失败
#define		__START_FILTER_CREATIVE							0xE0034000//**创意
#define		E_CREATIVE_PRICE_LOW							0xE0034001//[创意] 优先选择价高创意，价低者被忽略
#define		E_CREATIVE_PRICE_CALLBACK						0xE0034002//[创意] 回调函数检查创意价格不符合（各adx检测规则不一样，一般是创意价格至少比流量底价多一分）
#define		E_CREATIVE_DEEPLINK								0xE0034003//[创意] 流量要求deeplink，创意不支持
#define		E_CREATIVE_TYPE									0xE0034004//[创意] 类型不匹配
#define		E_CREATIVE_SIZE									0xE0034005//[创意] 创意（这里只有banner或video，native单算） 尺寸不符合
#define		E_CREATIVE_EXTS									0xE0034010//[创意] 创意的ext字段不符合（目前为创意信息里没有设置审核过的创意id）
#define		E_CREATIVE_ADVCAT								0xE0034020//[创意] 广告行业关联度不是最匹配
#define		E_CREATIVE_LAST_RANDOM							0xE0034030//[创意] 最后的几个合适的创意里，只随机取一个，剩余的不被选中
#define		E_CREATIVE_UNSUPPORTED							0xE0034100//[创意] 广告类型不支持（支持banner,video,native）
#define		E_CREATIVE_BANNER_BTYPE							0xE0034111//[创意] banner 创意类型不符合（流量设置了黑名单）
#define		E_CREATIVE_BANNER_BATTR							0xE0034112//[创意] banner 属性不符合（流量设置了黑名单）
#define		E_CREATIVE_VIDEO_FTYPE							0xE0034200//[创意] 该创意 文件类型 不适合（视频类）
#define		E_CREATIVE_NATIVE_BCTYPE						0xE0034300//[创意] 原生广告 点击类型不符合
#define		E_CREATIVE_NATIVE_ASSET_TYPE					0xE0034310//[创意] 原生广告 元素类型不符合
#define		E_CREATIVE_NATIVE_ASSET_TITLE_LENGTH			0xE0034320//[创意] 原生广告 title的长度不符合
#define		E_CREATIVE_NATIVE_ASSET_IMAGE_TYPE				0xE0034330//[创意] 原生广告 image广告元素的类型不符合（只是icon或者大图）
#define		E_CREATIVE_NATIVE_ASSET_IMAGE_ICON_SIZE			0xE0034331//[创意] 原生广告 icon的尺寸不符合
#define		E_CREATIVE_NATIVE_ASSET_IMAGE_ICON_FTYPE		0xE0034332//[创意] 原生广告 icon的图片类型不符合
#define		E_CREATIVE_NATIVE_ASSET_IMAGE_ICON_URL			0xE0034333//[创意] 原生广告 icon的素材地址为空
#define		E_CREATIVE_NATIVE_ASSET_IMAGE_MAIN_SIZE			0xE0034334//[创意] 原生广告 大图的尺寸不符合
#define		E_CREATIVE_NATIVE_ASSET_IMAGE_MAIN_FTYPE		0xE0034335//[创意] 原生广告 大图的图片类型不符合
#define		E_CREATIVE_NATIVE_ASSET_IMAGE_MAIN_URL			0xE0034336//[创意] 原生广告 大图的素材地址为空
#define		E_CREATIVE_NATIVE_ASSET_IMAGE_NUM				0xE0034337//[创意] 原生广告 大图的数量与创意中大图数量不匹配
#define		E_CREATIVE_NATIVE_ASSET_DATA_DESC_LENGTH		0xE0034340//[创意] 原生广告 描述信息长度不符合
#define		E_CREATIVE_NATIVE_ASSET_DATA_RATING_VALUE		0xE0034341//[创意] 原生广告 rating的值大于5
#define		E_CREATIVE_NATIVE_ASSET_DATA_CTATEXT			0xE0034342//[创意] 原生广告 ctatext为空
#define		__END_FILTER									0xE003FFFF


// *E* 后期处理 0xE0040000 - 0xE004FFFF
#define		__START_LATER									0xE0040000
#define		E_ADXTEMPLATE_NOT_FIND_IURL						0xE0040001//[后期处理] 模板中没有iurl
#define		E_ADXTEMPLATE_NOT_FIND_CTURL					0xE0040002//[后期处理] 模板中没有cturl
#define		E_ADXTEMPLATE_NOT_FIND_AURL						0xE0040003//[后期处理] 模板中没有aurl
#define		E_ADXTEMPLATE_NOT_FIND_NURL						0xE0040004//[后期处理] 模板中没有nurl
#define		E_ADXTEMPLATE_NOT_FIND_ADM						0xE0040005//[后期处理] 模板中没有adm
#define		E_ADXTEMPLATE_RATIO_ZERO						0xE0040006//[后期处理] 模板中的价格比例是0
#define		__END_LATER										0xE004FFFF


////////////////////////////////////////////////////////////////////////////////////////////
#define		E_TRACK									0xE0090000//追踪系统的错误
#define		E_TRACK_EMPTY							0xE0091000//空错误
#define		E_TRACK_EMPTY_CPU_COUNT					0xE0091001//[flume]cpu_count为空
#define		E_TRACK_EMPTY_FLUME_IP					0xE0091002//[flume]flume_ip为空
#define		E_TRACK_EMPTY_FLUME_PORT				0xE0091003//[flume]flume_port为空
#define		E_TRACK_EMPTY_KAFKA_TOPIC				0xE0091004//[flume]kafka_topic为空
#define		E_TRACK_EMPTY_KAFKA_BROKER_LIST			0xE0091005//[flume]kafka_broker_list为空
#define		E_TRACK_EMPTY_MASTER_UR_IP				0xE0091006//[flume]master_ur_ip为空
#define		E_TRACK_EMPTY_MASTER_UR_PORT			0xE0091007//[flume]master_ur_port为空
#define		E_TRACK_EMPTY_SLAVE_UR_IP				0xE0091008//[flume]slave_ur_ip为空
#define		E_TRACK_EMPTY_SLAVE_UR_PORT				0xE0091009//[flume]slave_ur_port为空
#define		E_TRACK_EMPTY_MASTER_GROUP_IP			0xE0091010//[flume]master_group_ip为空
#define		E_TRACK_EMPTY_MASTER_GROUP_PORT			0xE0091011//[flume]master_group_port为空
#define		E_TRACK_EMPTY_SLAVE_GROUP_IP			0xE0091012//[flume]slave_group_ip为空
#define		E_TRACK_EMPTY_SLAVE_GROUP_PORT			0xE0091013//[flume]slave_group_port为空
#define		E_TRACK_EMPTY_MASTER_MAJOR_IP			0xE0091014//[flume]master_major_ip为空
#define		E_TRACK_EMPTY_MASTER_MAJOR_PORT			0xE0091015//[flume]master_major_port为空
#define		E_TRACK_EMPTY_SLAVE_MAJOR_IP			0xE0091016//[flume]slave_major_ip为空
#define		E_TRACK_EMPTY_SLAVE_MAJOR_PORT			0xE0091017//[flume]slave_major_port为空
#define		E_TRACK_EMPTY_MASTER_FILTER_ID_IP		0xE0091018//[flume]master_filter_id_ip为空
#define		E_TRACK_EMPTY_MASTER_FILTER_ID_PORT		0xE0091019//[flume]master_filter_id_port为空
#define		E_TRACK_EMPTY_SLAVE_FILTER_ID_IP		0xE0091020//[flume]slave_filter_id_ip为空
#define		E_TRACK_EMPTY_SLAVE_FILTER_ID_PORT		0xE0091021//[flume]slave_filter_id_port为空
#define		E_TRACK_EMPTY_MTYPE						0xE0091022//[flume]mtype(请求的类型)为空
#define		E_TRACK_EMPTY_ADX						0xE0091023//[flume]adx(请求的渠道id)为空
#define		E_TRACK_EMPTY_MAPID						0xE0091024//[flume]mapid(创意id)为空
#define		E_TRACK_EMPTY_BID						0xE0091025//[flume]bid(请求id)为空
#define		E_TRACK_EMPTY_IMPID						0xE0091026//[flume]impid(广告展示机会id)为空
#define		E_TRACK_EMPTY_GETCONTEXTINFO_HANDLE		0xE0091027//[flume]去重redis句柄为空
#define		E_TRACK_EMPTY_PRICE						0xE0091028//[flume]赢价或展现通知价格为空
#define		E_TRACK_EMPTY_ADVERTISER				0xE0091029//[flume]advertiser(广告主)为空
#define		E_TRACK_EMPTY_DEVICEID					0xE0091030//[flume]deviceid(设备id)为空
#define		E_TRACK_EMPTY_DEVICEIDTYPE				0xE0091031//[flume]deviceidtype(设备id类型)为空
#define		E_TRACK_EMPTY_BID_PRICE					0xE0091032//[flume]缓存bid对应的价格为空
#define		E_TRACK_EMPTY_PRICE_CEILING				0xE0091033//[flume]价格上限为空
#define		E_TRACK_EMPTY_REMOTEADDR				0xE0091034//[flume]请求地址为空
#define		E_TRACK_EMPTY_REQUESTPARAM				0xE0091035//[flume]请求参数为空

#define		E_TRACK_MALLOC							0xE0092000//分配空间错误
#define		E_TRACK_MALLOC_PID						0xE0092001//[flume]为pid分配空间错误
#define		E_TRACK_MALLOC_DATATRANSFER_K			0xE0092002//[flume]为datatransfer_kafka分配空间错误
#define		E_TRACK_MALLOC_DATATRANSFER_F			0xE0092003//[flume]为datatransfer_flume分配空间错误
#define		E_TRACK_MALLOC_KAFKADATA				0xE0092004//[flume]为kafkadata分配空间错误

#define		E_TRACK_INVALID							0xE0093000//无效错误
#define		E_TRACK_INVALID_FLUME_LOGID				0xE0093001//[flume]无效的flume日志句柄
#define		E_TRACK_INVALID_KAFKA_LOGID				0xE0093002//[flume]无效的kafka日志句柄
#define		E_TRACK_INVALID_LOCAL_LOGID				0xE0093003//[flume]无效的local日志句柄
#define		E_TRACK_INVALID_MUTEX_IDFLAG			0xE0093004//[flume]无效的去重信息redis互斥锁
#define		E_TRACK_INVALID_MUTEX_USER				0xE0093005//[flume]无效的用户信息redis互斥锁
#define		E_TRACK_INVALID_MUTEX_GROUP				0xE0093006//[flume]无效的组信息redis互斥锁
#define		E_TRACK_INVALID_MONITORCONTEXT_HANDLE	0xE0093007//[flume]无效的监视器句柄
#define		E_TRACK_INVALID_DATATRANSFER_K			0xE0093008//[flume]无效的datatransfer_kafka
#define		E_TRACK_INVALID_DATATRANSFER_F			0xE0093009//[flume]无效的datatransfer_flume
#define		E_TRACK_INVALID_REQUEST_ADDRESS			0xE0093010//[flume]无效的请求ip
#define		E_TRACK_INVALID_REQUEST_PARAMETERS		0xE0093011//[flume]无效的请求参数集
#define		E_TRACK_INVALID_GETCONTEXTINFO_HANDLE	0xE0093012//[flume]无效的去重redis句柄
#define		E_TRACK_INVALID_PRICE_DECODE_HANDLE		0xE0093013//[flume]无效的价格解密句柄
#define		E_TRACK_INVALID_WINNOTICE				0xE0093014//[flume]无效的赢价通知(bid_flag value 为空)
#define		E_TRACK_INVALID_IMPNOTICE				0xE0093015//[kafka]无效的展现通知(bid_flag value 为空)
#define		E_TRACK_INVALID_CLKNOTICE				0xE0093016//[kafka]无效的点击通知(bid_flag value 为空)
	
#define		E_TRACK_UNDEFINE						0xE0094000//未定义错误
#define		E_TRACK_UNDEFINE_MAPID					0xE0094001//[flume]未定义的mapid(格式错误)
#define		E_TRACK_UNDEFINE_NRTB					0xE0094002//[flume]未定义的nrtb(值未定义)
#define		E_TRACK_UNDEFINE_AT						0xE0094003//[flume]未定义的at(值未定义)
#define		E_TRACK_UNDEFINE_MTYPE					0xE0094004//[flume]未定义的mtype(值未定义)
#define		E_TRACK_UNDEFINE_ADX					0xE0094005//[flume]未定义的adx(值未定义)
#define		E_TRACK_UNDEFINE_ADVERTISER				0xE0094006//[flume]未定义的advertiser(值未定义)
#define		E_TRACK_UNDEFINE_APPSID					0xE0094007//[flume]未定义的appsid(值未定义)
#define		E_TRACK_UNDEFINE_DEVICEID				0xE0094008//[flume]未定义的deviceid(长度超过40)
#define		E_TRACK_UNDEFINE_DEVICEIDTYPE			0xE0094009//[flume]未定义的deviceidtype(长度超过5)
#define		E_TRACK_UNDEFINE_PRICE_CEILING			0xE0094010//[flume]未定义的赢价价格上限(值>50元)

#define		E_TRACK_REPEATED						0xE0095000//重复错误
#define		E_TRACK_REPEATED_WINNOTICE				0xE0095001//[kafka]重复的赢价通知
#define		E_TRACK_REPEATED_IMPNOTICE				0xE0095002//[kafka]重复的展现通知
#define		E_TRACK_REPEATED_CLKNOTICE				0xE0095003//[kafka]重复的点击通知

#define		E_TRACK_FAILED							0xE0096000//失败错误
#define		E_TRACK_FAILED_DECODE_PRICE				0xE0096001//[flume]解析价格失败
#define		E_TRACK_FAILED_NORMALIZATION_PRICE		0xE0096002//[flume]归一化价格失败
#define		E_TRACK_FAILED_FCGX_ACCEPT_R			0xE0096003//[flume]fast-cgi框架接受连接失败
#define		E_TRACK_FAILED_SEND_FLUME				0xE0096004//[flume]发送flume失败
#define		E_TRACK_FAILED_SEND_KAFKA				0xE0096005//[flume]发送kafka失败
#define		E_TRACK_FAILED_FREQUENCYCAPPING			0xE0096006//[flume]展现或点击的frequency计数失败
#define		E_TRACK_FAILED_SEMAPHORE_WAIT			0xE0096007//[flume]semaphore_wait失败
#define		E_TRACK_FAILED_SEMAPHORE_RELEASE		0xE0096008//[flume]semaphore_release失败
#define		E_TRACK_FAILED_PARSE_DEVICEINFO			0xE0096009//[flume]解析deviceid和deviceidtype失败 

#define		E_TRACK_OTHER							0xE0097000//其他错误
#define		E_TRACK_OTHER_APP_MONITOR				0xE0097001//[flume]app下载相关监控
#define		E_TRACK_OTHER_UPPER_LIMIT_ALERT			0xE0097002//[flume]second price 超出上限警报

/////////////////////////////////////////////////////////////////////////////////////


#define		E_DL_GET_FUNCTION_FAIL					0xE0110000//结算部分的错误
#define		E_DL_OPEN_FAIL							0xE0111000//
#define		E_REPEATED_REQUEST						0xE0112000//重复请求
#define		E_DL_DECODE_FAILED						0xE0113000//价格解密失败

#endif
