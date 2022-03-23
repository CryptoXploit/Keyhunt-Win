/*
Develop by Luis Alberto
email: alberto.bsd@gmail.com
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <vector>
#include <inttypes.h>
#include "base58/libbase58.h"
#include "rmd160/rmd160.h"
#include "oldbloom/oldbloom.h"
#include "bloom/bloom.h"
#include "sha3/sha3.h"
#include "util.h"

#include "secp256k1/SECP256k1.h"
#include "secp256k1/Point.h"
#include "secp256k1/Int.h"
#include "secp256k1/IntGroup.h"
#include "secp256k1/Random.h"

#include "hash/sha256.h"
#include "hash/ripemd160.h"

#if defined(_WIN64) && !defined(__CYGWIN__)
#include "getopt.h"
#include <windows.h>
#else
#include <unistd.h>
#include <pthread.h>
#endif

#define CRYPTO_NONE 0
#define CRYPTO_BTC 1
#define CRYPTO_ETH 2
#define CRYPTO_ALL 3

#define MODE_XPOINT 0
#define MODE_ADDRESS 1
#define MODE_BSGS 2
#define MODE_RMD160 3
#define MODE_PUB2RMD 4
#define MODE_MINIKEYS 5
//#define MODE_CHECK 6



#define SEARCH_UNCOMPRESS 0
#define SEARCH_COMPRESS 1
#define SEARCH_BOTH 2


#define FLIPBITLIMIT 10000000

uint32_t  THREADBPWORKLOAD = 1048576;

struct checksumsha256	{
	char data[32];
	char backup[32];
};

struct bsgs_xvalue	{
	uint8_t value[6];
	uint64_t index;
};

struct address_value	{
	uint8_t value[20];
};

struct tothread {
	int nt;     //Number thread
	char *rs;   //range start
	char *rpt;  //rng per thread
};

struct bPload	{
	uint32_t threadid;
	uint64_t from;
	uint64_t to;
	uint64_t counter;
	uint64_t workload;
	uint32_t aux;
	uint32_t finished;
};

#if defined(_WIN64) && !defined(__CYGWIN__)
#define PACK( __Declaration__ ) __pragma( pack(push, 1) ) __Declaration__ __pragma( pack(pop))
PACK(struct publickey
{
	uint8_t parity;
	union {
		uint8_t data8[32];
		uint32_t data32[8];
		uint64_t data64[4];
	} X;
});
#else
struct __attribute__((__packed__)) publickey {
  uint8_t parity;
	union	{
		uint8_t data8[32];
		uint32_t data32[8];
		uint64_t data64[4];
	} X;
};
#endif

const char *Ccoinbuffer_default = "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";

char *Ccoinbuffer = (char*) Ccoinbuffer_default;
char *str_baseminikey = NULL;
char *raw_baseminikey = NULL;
char *minikeyN = NULL;
int minikey_n_limit;
	
const char *version = "0.2.211117 SSE Trick or treat ¡Beta!";

#define CPU_GRP_SIZE 1024

std::vector<Point> Gn;
Point _2Gn;

std::vector<Point> GSn;
Point _2GSn;

std::vector<Point> GSn2;
Point _2GSn2;

std::vector<Point> GSn3;
Point _2GSn3;


void init_generator();

int searchbinary(struct address_value *buffer,char *data,int64_t array_length);
void sleep_ms(int milliseconds);

void _sort(struct address_value *arr,int64_t N);
void _insertionsort(struct address_value *arr, int64_t n);
void _introsort(struct address_value *arr,uint32_t depthLimit, int64_t n);
void _swap(struct address_value *a,struct address_value *b);
int64_t _partition(struct address_value *arr, int64_t n);
void _myheapsort(struct address_value	*arr, int64_t n);
void _heapify(struct address_value *arr, int64_t n, int64_t i);

void bsgs_sort(struct bsgs_xvalue *arr,int64_t n);
void bsgs_myheapsort(struct bsgs_xvalue *arr, int64_t n);
void bsgs_insertionsort(struct bsgs_xvalue *arr, int64_t n);
void bsgs_introsort(struct bsgs_xvalue *arr,uint32_t depthLimit, int64_t n);
void bsgs_swap(struct bsgs_xvalue *a,struct bsgs_xvalue *b);
void bsgs_heapify(struct bsgs_xvalue *arr, int64_t n, int64_t i);
int64_t bsgs_partition(struct bsgs_xvalue *arr, int64_t n);

int bsgs_searchbinary(struct bsgs_xvalue *arr,char *data,int64_t array_length,uint64_t *r_value);
int bsgs_secondcheck(Int *start_range,uint32_t a,uint32_t k_index,Int *privatekey);
int bsgs_thirdcheck(Int *start_range,uint32_t a,uint32_t k_index,Int *privatekey);

void sha256sse_22(uint8_t *src0, uint8_t *src1, uint8_t *src2, uint8_t *src3, uint8_t *dst0, uint8_t *dst1, uint8_t *dst2, uint8_t *dst3);
void sha256sse_23(uint8_t *src0, uint8_t *src1, uint8_t *src2, uint8_t *src3, uint8_t *dst0, uint8_t *dst1, uint8_t *dst2, uint8_t *dst3);

#if defined(_WIN64) && !defined(__CYGWIN__)
DWORD WINAPI thread_process_minikeys(LPVOID vargp);
DWORD WINAPI thread_process(LPVOID vargp);
DWORD WINAPI thread_process_bsgs(LPVOID vargp);
DWORD WINAPI thread_process_bsgs_backward(LPVOID vargp);
DWORD WINAPI thread_process_bsgs_both(LPVOID vargp);
DWORD WINAPI thread_process_bsgs_random(LPVOID vargp);
DWORD WINAPI thread_process_bsgs_dance(LPVOID vargp);
DWORD WINAPI thread_bPload(LPVOID vargp);
DWORD WINAPI thread_bPload_2blooms(LPVOID vargp);
DWORD WINAPI thread_pub2rmd(LPVOID vargp);
//DWORD WINAPI thread_process_check_file_btc(LPVOID vargp);
#else
void *thread_process_minikeys(void *vargp);	
void *thread_process(void *vargp);
void *thread_process_bsgs(void *vargp);
void *thread_process_bsgs_backward(void *vargp);
void *thread_process_bsgs_both(void *vargp);
void *thread_process_bsgs_random(void *vargp);
void *thread_process_bsgs_dance(void *vargp);
void *thread_bPload(void *vargp);
void *thread_bPload_2blooms(void *vargp);
void *thread_pub2rmd(void *vargp);
//void *thread_process_check_file_btc(void *vargp);
#endif

char *publickeytohashrmd160(char *pkey,int length);
void publickeytohashrmd160_dst(char *pkey,int length,char *dst);
char *pubkeytopubaddress(char *pkey,int length);
void pubkeytopubaddress_dst(char *pkey,int length,char *dst);
void rmd160toaddress_dst(char *rmd,char *dst);
void set_minikey(char *buffer,char *rawbuffer,int length);
bool increment_minikey_index(char *buffer,char *rawbuffer,int index);
void increment_minikey_N(char *rawbuffer);
	
void KECCAK_256(uint8_t *source, size_t size,uint8_t *dst);
void generate_binaddress_eth(Point &publickey,unsigned char *dst_address);
void memorycheck_bsgs();

int THREADOUTPUT = 0;
char *bit_range_str_min;
char *bit_range_str_max;

const char *bsgs_modes[5] {"secuential","backward","both","random","dance"};
const char *modes[6] = {"xpoint","address","bsgs","rmd160","pub2rmd","minikeys"};
const char *cryptos[3] = {"btc","eth","all"};
const char *publicsearch[3] = {"uncompress","compress","both"};
const char *default_filename = "addresses.txt";

#if defined(_WIN64) && !defined(__CYGWIN__)
HANDLE* tid = NULL;
HANDLE write_keys;
HANDLE write_random;
HANDLE bsgs_thread;
HANDLE *bPload_mutex;
#else
pthread_t *tid = NULL;
pthread_mutex_t write_keys;
pthread_mutex_t write_random;
pthread_mutex_t bsgs_thread;
pthread_mutex_t *bPload_mutex;
#endif


uint64_t FINISHED_THREADS_COUNTER = 0;
uint64_t FINISHED_THREADS_BP = 0;
uint64_t THREADCYCLES = 0;
uint64_t THREADCOUNTER = 0;
uint64_t FINISHED_ITEMS = 0;
uint64_t OLDFINISHED_ITEMS = -1;


uint8_t byte_encode_crypto = 0x00;		/* Bitcoin  */
//uint8_t byte_encode_crypto = 0x1E;	/* Dogecoin */

struct bloom bloom;

uint64_t *steps = NULL;
unsigned int *ends = NULL;
uint64_t N = 0;


uint64_t N_SECUENTIAL_MAX = 0x100000000;
uint64_t DEBUGCOUNT = 0x400;
uint64_t u64range;

Int OUTPUTSECONDS;

int FLAGBASEMINIKEY = 0;
int FLAGBSGSMODE = 0;
int FLAGDEBUG = 0;
int FLAGQUIET = 0;
int FLAGMATRIX = 0;
int KFACTOR = 1;
int MAXLENGTHADDRESS = -1;
int NTHREADS = 1;

int FLAGSAVEREADFILE = 0;
int FLAGREADEDFILE1 = 0;
int FLAGREADEDFILE2 = 0;
int FLAGREADEDFILE3 = 0;
int FLAGREADEDFILE4 = 0;

int FLAGUPDATEFILE1 = 0;


int FLAGSTRIDE = 0;
int FLAGSEARCH = 2;
int FLAGBITRANGE = 0;
int FLAGRANGE = 0;
int FLAGFILE = 0;
int FLAGVANITY = 0;
int FLAGMODE = MODE_ADDRESS;
int FLAGCRYPTO = 0;
int FLAGALREADYSORTED = 0;
int FLAGRAWDATA	= 0;
int FLAGRANDOM = 0;
int FLAG_N = 0;
int FLAGPRECALCUTED_P_FILE = 0;
int COUNT_VANITIES = 0;

int *len_vanities;
int bitrange;
char *str_N;
char **vanities;
char *range_start;
char *range_end;
char *str_stride;
Int stride;

uint64_t BSGS_XVALUE_RAM = 6;
uint64_t BSGS_BUFFERXPOINTLENGTH = 32;
uint64_t BSGS_BUFFERREGISTERLENGTH = 36;

/*
BSGS Variables
*/
int *bsgs_found;
std::vector<Point> OriginalPointsBSGS;
bool *OriginalPointsBSGScompressed;

uint64_t bytes;
char checksum[32],checksum_backup[32];
char buffer_bloom_file[1024];
struct bsgs_xvalue *bPtable;
struct address_value *addressTable;

struct oldbloom oldbloom_bP;

struct bloom *bloom_bP;
struct bloom *bloom_bPx2nd; //2nd Bloom filter check
struct bloom *bloom_bPx3rd; //3rd Bloom filter check

struct checksumsha256 *bloom_bP_checksums;
struct checksumsha256 *bloom_bPx2nd_checksums;
struct checksumsha256 *bloom_bPx3rd_checksums;

#if defined(_WIN64) && !defined(__CYGWIN__)
std::vector<HANDLE> bloom_bP_mutex;
std::vector<HANDLE> bloom_bPx2nd_mutex;
std::vector<HANDLE> bloom_bPx3rd_mutex;
#else
pthread_mutex_t *bloom_bP_mutex;
pthread_mutex_t *bloom_bPx2nd_mutex;
pthread_mutex_t *bloom_bPx3rd_mutex;
#endif




uint64_t bloom_bP_totalbytes = 0;
uint64_t bloom_bP2_totalbytes = 0;
uint64_t bloom_bP3_totalbytes = 0;
uint64_t bsgs_m = 4194304;
uint64_t bsgs_m2;
uint64_t bsgs_m3;
unsigned long int bsgs_aux;
uint32_t bsgs_point_number;

const char *str_limits_prefixs[7] = {"Mkeys/s","Gkeys/s","Tkeys/s","Pkeys/s","Ekeys/s","Zkeys/s","Ykeys/s"};
const char *str_limits[7] = {"1000000","1000000000","1000000000000","1000000000000000","1000000000000000000","1000000000000000000000","1000000000000000000000000"};
Int int_limits[7];




Int BSGS_GROUP_SIZE;
Int BSGS_CURRENT;
Int BSGS_R;
Int BSGS_AUX;
Int BSGS_N;
Int BSGS_M;					//M is squareroot(N)
Int BSGS_M2;
Int BSGS_M3;
Int ONE;
Int ZERO;
Int MPZAUX;

Point BSGS_P;			//Original P is actually G, but this P value change over time for calculations
Point BSGS_MP;			//MP values this is m * P
Point BSGS_MP2;			//MP2 values this is m2 * P
Point BSGS_MP3;			//MP3 values this is m3 * P

std::vector<Point> BSGS_AMP2;
std::vector<Point> BSGS_AMP3;

Point point_temp,point_temp2;	//Temp value for some process

Int n_range_start;
Int n_range_end;
Int n_range_diff;
Int n_range_aux;

Secp256K1 *secp;

int main(int argc, char **argv)	{
	char buffer[2048];
	char temporal[65];
	char rawvalue[32];
	struct tothread *tt;	//tothread
	Tokenizer t,tokenizerbsgs,tokenizer_xpoint;	//tokenizer
	char *filename = NULL;
	char *precalculated_mp_filename = NULL;
	char *hextemp = NULL;
	char *aux = NULL;
	char *aux2 = NULL;
	char *pointx_str = NULL;
	char *pointy_str = NULL;
	char *str_seconds = NULL;
	char *str_total = NULL;
	char *str_pretotal = NULL;
	char *str_divpretotal = NULL;
	char *bf_ptr = NULL;
	char *bPload_threads_available;
	FILE *fd,*fd_aux1,*fd_aux2,*fd_aux3;
	uint64_t j,total_precalculated,i,PERTHREAD,BASE,PERTHREAD_R,itemsbloom,itemsbloom2,itemsbloom3;
	uint32_t finished;
	int readed,continue_flag,check_flag,r,lenaux,lendiff,c,salir,index_value;
	Int total,pretotal,debugcount_mpz,seconds,div_pretotal,int_aux,int_r,int_q,int58;
	struct bPload *bPload_temp_ptr;
	
#if defined(_WIN64) && !defined(__CYGWIN__)
	DWORD s;
	write_keys = CreateMutex(NULL, FALSE, NULL);
	write_random = CreateMutex(NULL, FALSE, NULL);
	bsgs_thread = CreateMutex(NULL, FALSE, NULL);
#else
	pthread_mutex_init(&write_keys,NULL);
	pthread_mutex_init(&write_random,NULL);
	pthread_mutex_init(&bsgs_thread,NULL);
	int s;
#endif

	srand (time(NULL));

	secp = new Secp256K1();
	secp->Init();
	OUTPUTSECONDS.SetInt32(30);
	ZERO.SetInt32(0);
	ONE.SetInt32(1);
	BSGS_GROUP_SIZE.SetInt32(CPU_GRP_SIZE);
	rseed(clock() + time(NULL));
	
#if defined(_WIN64) && !defined(__CYGWIN__)
	printf("[+] Version %s, developed by AlbertoBSD(Win64 build by KV)\n", version);
#else
	printf("[+] Version %s, developed by AlbertoBSD\n",version);
#endif
	

	while ((c = getopt(argc, argv, "dehMqRwzSB:b:c:C:E:f:I:k:l:m:N:n:p:r:s:t:v:V:G:8:")) != -1) {
		switch(c) {
			case 'h':
				printf("\nUsage:\n-h\t\tshow this help\n");
				printf("-B Mode\t\tBSGS now have some modes <secuential,backward,both,random,dance>\n");
				printf("-b bits\t\tFor some puzzles you only need some numbers of bits in the test keys.\n");
				printf("-c crypto\tSearch for specific crypo. < btc, eth, all > valid only w/ -m address \n");
				printf("-C mini\t\tSet the minikey Base only 22 character minikeys, ex: SRPqx8QiwnW4WNWnTVa2W5\n");
				printf("-8 alpha\t\tSet the bas58 alphabet for minikeys");
				printf("-f file\t\tSpecify filename with addresses or xpoints or uncompressed public keys\n");
				printf("-I stride\tStride for xpoint, rmd160 and address, this option don't work with bsgs	\n");
				printf("-k value\tUse this only with bsgs mode, k value is factor for M, more speed but more RAM use wisely\n");
				printf("-l look\tWhat type of address/hash160 are you looking for < compress , uncompress , both> Only for rmd160 and address\n");
				printf("-m mode\t\tmode of search for cryptos. ( bsgs , xpoint , rmd160 , address ) default: address (more slow)\n");
				printf("-M\t\tMatrix screen, feel like a h4x0r, but performance will droped\n");
				printf("-n uptoN\tCheck for N secuential numbers before the random chossen this only work with -R option\n");
				printf("\t\tUse -n to set the N for the BSGS process. Bigger N more RAM needed\n");
				printf("-q\t\tQuiet the thread output\n");
				printf("-r SR:EN\tStarRange:EndRange, the end range can be omited for search from start range to N-1 ECC value\n");
				printf("-R\t\tRandom this is the default behaivor\n");
				printf("-s ns\t\tNumber of seconds for the stats output, 0 to omit output.\n");
				printf("-S\t\tCapital S is for SAVING in files BSGS data (Bloom filters and bPtable)\n");
				printf("-t tn\t\tThreads number, must be positive integer\n");
				printf("-v va\t\tSearch for vanity Address, only with -m address\n");
				printf("-V file\t\tFile with vanity Address to search, only with -m address");
				printf("-w\t\tMark the input file as RAW data xpoint fixed 32 byte each point. Valid only with -m xpoint\n");
				printf("\nExample\n\n");
				printf("%s -t 16 -r 1:FFFFFFFF -s 0\n\n",argv[0]);
				printf("This line run the program with 16 threads from the range 1 to FFFFFFFF without stats output\n\n");
				printf("Developed by AlbertoBSD\tTips BTC: 1ABSD1rMTmNZHJrJP8AJhDNG1XbQjWcRz7\n");
				printf("Thanks to Iceland always helping and sharing his ideas.\nTips to Iceland: bc1q39meky2mn5qjq704zz0nnkl0v7kj4uz6r529at\n\n");
				exit(0);
			break;
			case 'B':
				index_value = indexOf(optarg,bsgs_modes,5);
				if(index_value >= 0 && index_value <= 4)	{
					FLAGBSGSMODE = index_value;
					//printf("[+] BSGS mode %s\n",optarg);
				}
				else	{
					fprintf(stderr,"[W] Ignoring unknow bsgs mode %s\n",optarg);
				}
			break;
			case 'b':
				bitrange = strtol(optarg,NULL,10);
				if(bitrange > 0 && bitrange <=256 )	{
					MPZAUX.Set(&ONE);
					MPZAUX.ShiftL(bitrange-1);
					bit_range_str_min = MPZAUX.GetBase16();
					MPZAUX.Set(&ONE);
					MPZAUX.ShiftL(bitrange);
					if(MPZAUX.IsGreater(&secp->order))	{
						MPZAUX.Set(&secp->order);
					}
					bit_range_str_max = MPZAUX.GetBase16();
					if(bit_range_str_min == NULL||bit_range_str_max == NULL)	{
						fprintf(stderr,"[E] error malloc()\n");
						exit(0);
					}
					FLAGBITRANGE = 1;
				}
				else	{
					fprintf(stderr,"[E] invalid bits param: %s.\n",optarg);
				}
			break;
			case 'c':
				index_value = indexOf(optarg,cryptos,3);
				switch(index_value) {
					case 0: //btc
						FLAGCRYPTO = CRYPTO_BTC;
						printf("[+] Setting search for BTC adddress.\n");
					break;
					case 1: //eth
						FLAGCRYPTO = CRYPTO_ETH;
						printf("[+] Setting search for ETH adddress.\n");
					break;
					case 2: //all
						FLAGCRYPTO = CRYPTO_ALL;
						printf("[+] Setting search for all cryptocurrencies avaible [btc].\n");
					break;
					default:
						FLAGCRYPTO = CRYPTO_NONE;
						fprintf(stderr,"[E] Unknow crypto value %s\n",optarg);
						exit(0);
					break;
				}
			break;
			case 'C':
				if(strlen(optarg) == 22)	{
					FLAGBASEMINIKEY = 1;
					str_baseminikey = (char*) malloc(23);
					raw_baseminikey = (char*) malloc(23);
					if(str_baseminikey == NULL || raw_baseminikey == NULL)	{
						fprintf(stderr,"[E] malloc()\n");
					}
					strncpy(str_baseminikey,optarg,22);
					for(i = 0; i< 21; i++)	{
						if(strchr(Ccoinbuffer,str_baseminikey[i+1]) != NULL)	{
							raw_baseminikey[i] = (int)(strchr(Ccoinbuffer,str_baseminikey[i+1]) - Ccoinbuffer) % 58;
						}
						else	{
							fprintf(stderr,"[E] invalid character in minikey\n");
							exit(0);
						}
						
					}
				}
				else	{
					fprintf(stderr,"[E] Invalid Minikey length %i : %s\n",strlen(optarg),optarg);
					exit(0);
				}
				
			break;
			case 'd':
				FLAGDEBUG = 1;
				printf("[+] Flag DEBUG enabled\n");
			break;

			case 'e':
				FLAGALREADYSORTED = 1;
			break;
			case 'f':
				FLAGFILE = 1;
				filename = optarg;
			break;
			case 'I':
				FLAGSTRIDE = 1;
				str_stride = optarg;
			break;
						

			case 'k':
				KFACTOR = (int)strtol(optarg,NULL,10);
				if(KFACTOR <= 0)	{
					KFACTOR = 1;
				}
				printf("[+] K factor %i\n",KFACTOR);
			break;

			case 'l':
				switch(indexOf(optarg,publicsearch,3)) {
					case SEARCH_UNCOMPRESS:
						FLAGSEARCH = SEARCH_UNCOMPRESS;
						printf("[+] Search uncompress only\n");
					break;
					case SEARCH_COMPRESS:
						FLAGSEARCH = SEARCH_COMPRESS;
						printf("[+] Search compress only\n");
					break;
					case SEARCH_BOTH:
						FLAGSEARCH = SEARCH_BOTH;
						printf("[+] Search both compress and uncompress\n");
					break;
				}
			break;
			case 'M':
				FLAGMATRIX = 1;
				printf("[+] Matrix screen\n");
			break;
			case 'm':
				switch(indexOf(optarg,modes,6)) {
					case MODE_XPOINT: //xpoint
						FLAGMODE = MODE_XPOINT;
						printf("[+] Mode xpoint\n");
					break;
					case MODE_ADDRESS: //address
						FLAGMODE = MODE_ADDRESS;
						printf("[+] Mode address\n");
					break;
					case MODE_BSGS:
						FLAGMODE = MODE_BSGS;
						//printf("[+] Mode BSGS\n");
					break;
					case MODE_RMD160:
						FLAGMODE = MODE_RMD160;
						FLAGCRYPTO = CRYPTO_BTC;
						printf("[+] Mode rmd160\n");
					break;
					case MODE_PUB2RMD:
						FLAGMODE = MODE_PUB2RMD;
						printf("[+] Mode pub2rmd\n");
					break;
					case MODE_MINIKEYS:
						FLAGMODE = MODE_MINIKEYS;
						printf("[+] Mode minikeys\n");
					break;
					/*
					case MODE_CHECK:
						FLAGMODE = MODE_CHECK;
						printf("[+] Mode CHECK\n");
					break;
					*/
					default:
						fprintf(stderr,"[E] Unknow mode value %s\n",optarg);
						exit(0);
					break;
				}
			break;
			case 'n':
				FLAG_N = 1;
				str_N = optarg;
			break;

			case 'q':
				FLAGQUIET	= 1;
				printf("[+] Quiet thread output\n");
			break;
			case 'R':
				printf("[+] Random mode\n");
				FLAGRANDOM = 1;
				FLAGBSGSMODE =  3;
			break;
			case 'r':
				if(optarg != NULL)	{
					stringtokenizer(optarg,&t);
					switch(t.n)	{
						case 1:
							range_start = nextToken(&t);
							if(isValidHex(range_start)) {
								FLAGRANGE = 1;
								range_end = secp->order.GetBase16();
							}
							else	{
								fprintf(stderr,"[E] Invalid hexstring : %s.\n",range_start);
							}
						break;
						case 2:
							range_start = nextToken(&t);
							range_end	 = nextToken(&t);
							if(isValidHex(range_start) && isValidHex(range_end)) {
									FLAGRANGE = 1;
							}
							else	{
								if(isValidHex(range_start)) {
									printf("[E] Invalid hexstring : %s\n",range_start);
								}
								else	{
									printf("[E] Invalid hexstring : %s\n",range_end);
								}
							}
						break;
						default:
							printf("[E] Unknow number of Range Params: %i\n",t.n);
						break;
					}
				}
			break;
			case 's':
				OUTPUTSECONDS.SetBase10(optarg);
				if(OUTPUTSECONDS.IsLower(&ZERO))	{
					OUTPUTSECONDS.SetInt32(30);
				}
				if(OUTPUTSECONDS.IsZero())	{
					printf("[+] Turn off stats output\n");
				}
				else	{
					hextemp = OUTPUTSECONDS.GetBase10();
					printf("[+] Stats output every %s seconds\n",hextemp);
					free(hextemp);
				}
			break;
			case 'S':
				FLAGSAVEREADFILE = 1;
			break;
			case 't':
				NTHREADS = strtol(optarg,NULL,10);
				if(NTHREADS <= 0)	{
					NTHREADS = 1;
				}
				printf((NTHREADS > 1) ? "[+] Threads : %u\n": "[+] Thread : %u\n",NTHREADS);
			break;
			case 'v':
				FLAGVANITY = 1;
				if(COUNT_VANITIES > 0)	{
					vanities = (char**)realloc(vanities,(COUNT_VANITIES +1 ) * sizeof(char*));
					len_vanities = (int*)realloc(len_vanities,(COUNT_VANITIES +1 ) * sizeof(int));
				}
				else	{
					vanities = (char**)malloc(sizeof(char*));
					len_vanities = (int*)malloc(sizeof(int));
				}
				if(vanities == NULL || len_vanities == NULL)	{
					fprintf(stderr,"[E] malloc!\n");
					exit(0);
				}
				len_vanities[COUNT_VANITIES] = strlen(optarg);
				vanities[COUNT_VANITIES] = (char*)malloc(len_vanities[COUNT_VANITIES]+2);
				if(vanities[COUNT_VANITIES] == NULL)	{
					fprintf(stderr,"[E] malloc!\n");
					exit(0);
				}
				snprintf(vanities[COUNT_VANITIES],len_vanities[COUNT_VANITIES]+1 ,"%s",optarg);
				printf("[+] Added Vanity search : %s\n",vanities[COUNT_VANITIES]);
				COUNT_VANITIES++;
			break;
			case 'V':
				FLAGVANITY = 1;
				printf("[+] Added Vanity file : %s\n",optarg);
				fd = fopen(optarg,"r");
				if(fd == NULL)	{
					fprintf(stderr,"[E] Can't open the file : %s\n",optarg);
					exit(0);
				}
				r = 0;
				do	{
					fgets(buffer,1024,fd);
					trim(buffer,"\n\t\r ");
					if(strlen(buffer) > 0)	{
						r++;
					}
					memset(buffer,0,1024);
				}while(!feof(fd));
				
				if(r > 0)	{
					if(COUNT_VANITIES > 0)	{
						vanities = (char**)realloc(vanities,(COUNT_VANITIES + r ) * sizeof(char*));
						len_vanities = (int*)realloc(len_vanities,(COUNT_VANITIES + r )* sizeof(int));
					}
					else	{
						vanities = (char**)malloc( r  * sizeof(char*));
						len_vanities = (int*)malloc(r * sizeof(int));
					}
					if(vanities == NULL || len_vanities == NULL)	{
						fprintf(stderr,"[E] malloc!\n");
						exit(0);
					}
					
					fseek(fd,0,SEEK_SET);
					
					index_value = 0;
					while(index_value < r){
						fgets(buffer,1024,fd);
						trim(buffer,"\n\t\r ");
						if(strlen(buffer) > 0)	{
							len_vanities[COUNT_VANITIES+index_value] = strlen(buffer);
							vanities[COUNT_VANITIES+index_value] = (char*) malloc(len_vanities[COUNT_VANITIES+index_value]+2);
							if(vanities[COUNT_VANITIES+index_value] == NULL)	{
								fprintf(stderr,"[E] malloc!\n");
								exit(0);
							}
							snprintf(vanities[COUNT_VANITIES+index_value],len_vanities[COUNT_VANITIES+index_value]+1 ,"%s",buffer);
							printf("[D] Added Vanity search : %s\n",vanities[COUNT_VANITIES+index_value]);
						}
						memset(buffer,0,1024);
						index_value++;
					}
					COUNT_VANITIES+= r;
				}
				else	{
					fprintf(stderr,"[W] file %s is emptied\n",optarg);
					if(COUNT_VANITIES == 0)	{
						FLAGVANITY = 0;
					}
				}
				fclose(fd);
				fd = NULL;
			break;
			case 'w':
				printf("[+] Data marked as RAW\n");
				FLAGRAWDATA = 1;
			break;
			case '8':
				if(strlen(optarg) != 58)	{
					fprintf(stderr,"[W] The base58 alphabet must be 58 characters long.\n");
				}
				else	{
					Ccoinbuffer = optarg; 
					printf("[+] Base58 for Minikeys %s\n",Ccoinbuffer);
				}
			break;
			default:
				fprintf(stderr,"[E] Unknow opcion -%c\n",c);
			break;
		}
	}
	if( ( FLAGBSGSMODE == MODE_BSGS || FLAGBSGSMODE == MODE_PUB2RMD ) && FLAGSTRIDE == 1)	{
		fprintf(stderr,"[E] Stride doesn't work with BSGS, pub2rmd\n");
		exit(0);
	}
	if(FLAGSTRIDE)	{
		if(str_stride[0] == '0' && str_stride[1] == 'x')	{
			stride.SetBase16(str_stride+2);
		}
		else{
			stride.SetBase10(str_stride);
		}
		printf("[+] Stride : %s\n",stride.GetBase10());
	}
	else	{
		FLAGSTRIDE = 1;
		stride.Set(&ONE);
	}
	init_generator();
	if(FLAGMODE == MODE_BSGS )	{
		printf("[+] Mode BSGS %s\n",bsgs_modes[FLAGBSGSMODE]);
	}
	if(FLAGFILE == 0) {
		filename =(char*) default_filename;
	}
	printf("[+] Opening file %s\n",filename);
	fd = fopen(filename,"rb");
	if(fd == NULL)	{
		fprintf(stderr,"[E] Can't open file %s\n",filename);
		exit(0);
	}
	if(FLAGMODE == MODE_ADDRESS && FLAGCRYPTO == CRYPTO_NONE) {	//When none crypto is defined the default search is for Bitcoin
		FLAGCRYPTO = CRYPTO_BTC;
		printf("[+] Setting search for btc adddress\n");
	}
	/*
	if(FLAGCRYPTO == CRYPTO_ETH)	{
		FLAGCRYPTO = CRYPTO_BTC;
		printf("[+] Setting search for btc adddress\n");
		
	}
	*/
	if(FLAGRANGE) {
		n_range_start.SetBase16(range_start);
		if(n_range_start.IsZero())	{
			n_range_start.AddOne();
		}
		n_range_end.SetBase16(range_end);
		if(n_range_start.IsEqual(&n_range_end) == false ) {
			if(  n_range_start.IsLower(&secp->order) &&  n_range_end.IsLowerOrEqual(&secp->order) )	{
				if( n_range_start.IsGreater(&n_range_end)) {
					fprintf(stderr,"[W] Opps, start range can't be great than end range. Swapping them\n");
					n_range_aux.Set(&n_range_start);
					n_range_start.Set(&n_range_end);
					n_range_end.Set(&n_range_aux);
				}
				n_range_diff.Set(&n_range_end);
				n_range_diff.Sub(&n_range_start);
			}
			else	{
				fprintf(stderr,"[E] Start and End range can't be great than N\nFallback to random mode!\n");
				FLAGRANGE = 0;
			}
		}
		else	{
			fprintf(stderr,"[E] Start and End range can't be the same\nFallback to random mode!\n");
			FLAGRANGE = 0;
		}
	}
	if(FLAGMODE != MODE_BSGS && FLAGMODE != MODE_MINIKEYS)	{
		BSGS_N.SetInt32(DEBUGCOUNT);
		if(FLAGRANGE == 0 && FLAGBITRANGE == 0)	{
			n_range_start.SetInt32(1);
			n_range_end.Set(&secp->order);
			n_range_diff.Set(&n_range_end);
			n_range_diff.Sub(&n_range_start);
		}
		else	{
			if(FLAGBITRANGE)	{
				n_range_start.SetBase16(bit_range_str_min);
				n_range_end.SetBase16(bit_range_str_max);
				n_range_diff.Set(&n_range_end);
				n_range_diff.Sub(&n_range_start);
			}
			else	{
				if(FLAGRANGE == 0)	{
					fprintf(stderr,"[W] WTF!\n");
				}
			}
		}
	}
	N = 0;
	
	if(FLAGMODE != MODE_BSGS )	{
		if(FLAG_N){
			if(str_N[0] == '0' && str_N[1] == 'x')	{
				N_SECUENTIAL_MAX =strtol(str_N,NULL,16);
			}
			else	{
				N_SECUENTIAL_MAX =strtol(str_N,NULL,10);
			}
			
			if(N_SECUENTIAL_MAX < 1024)	{
				fprintf(stderr,"[I] n value need to be equal or great than 1024, back to defaults\n");
				FLAG_N = 0;
				N_SECUENTIAL_MAX = 0x100000000;
			}
			if(N_SECUENTIAL_MAX % 1024 != 0)	{
				fprintf(stderr,"[I] n value need to be multiplier of  1024\n");
				FLAG_N = 0;
				N_SECUENTIAL_MAX = 0x100000000;
			}
		}
		printf("[+] N = %p\n",(void*)N_SECUENTIAL_MAX);
		if(FLAGMODE == MODE_MINIKEYS)	{
			BSGS_N.SetInt32(DEBUGCOUNT);
			if(FLAGBASEMINIKEY)	{
				printf("[+] Base Minikey : %s\n",str_baseminikey);
				/*
				for(i = 0; i < 21;i ++)	{
					printf("%i ",(uint8_t) raw_baseminikey[i]);
				}
				printf("\n");
				*/
			}
			minikeyN = (char*) malloc(22);
			if(minikeyN == NULL)	{
				fprintf(stderr,"[E] malloc()\n");
				exit(0);
			}
			i =0;
			int58.SetInt32(58);
			int_aux.SetInt64(N_SECUENTIAL_MAX);
			int_aux.Mult(253);	
			/* We get approximately one valid mini key for each 256 candidates mini keys since this is only statistics we multiply N_SECUENTIAL_MAX by 253 to ensure not missed one one candidate minikey between threads... in this approach we repeat from 1 to 3 candidates in each N_SECUENTIAL_MAX cycle IF YOU FOUND some other workaround please let me know */
			i = 20;
			salir = 0;
			do	{
				if(!int_aux.IsZero())	{
					int_r.Set(&int_aux);
					int_r.Mod(&int58);
					int_q.Set(&int_aux);
					minikeyN[i] = (uint8_t)int_r.GetInt64();
					int_q.Sub(&int_r);
					int_q.Div(&int58);
					int_aux.Set(&int_q);
					i--;
				}
				else	{
					salir =1;
				}
			}while(!salir && i > 0);
			minikey_n_limit = 21 -i;
			/*
			for(i = 0; i < 21;i ++)	{
				printf("%i ",(uint8_t) minikeyN[i]);
			}
			printf(": minikey_n_limit %i\n",minikey_n_limit);
			*/
		}
		else	{
			if(FLAGBITRANGE)	{	// Bit Range
				printf("[+] Bit Range %i\n",bitrange);

			}
			else	{
				printf("[+] Range \n");
			}
		}
		if(FLAGMODE != MODE_MINIKEYS)	{
			hextemp = n_range_start.GetBase16();
			printf("[+] -- from : 0x%s\n",hextemp);
			free(hextemp);
			hextemp = n_range_end.GetBase16();
			printf("[+] -- to   : 0x%s\n",hextemp);
			free(hextemp);
		}
		
		aux =(char*) malloc(1000);
		if(aux == NULL)	{
			fprintf(stderr,"[E] error malloc()\n");
		}
		switch(FLAGMODE)	{
			case MODE_ADDRESS:
				/* We need to count how many lines are in the file */
					while(!feof(fd))	{
						hextemp = fgets(aux,998,fd);
						if(hextemp == aux)	{
							trim(aux," \t\n\r");
							r = strlen(aux);
							if(r > 10)	{ //Any length for invalid Address?
								if(r > MAXLENGTHADDRESS)	{
									MAXLENGTHADDRESS = r;
								}
								N++;
							}
						}
					}
					if(FLAGCRYPTO == CRYPTO_BTC)	{
						MAXLENGTHADDRESS = 32;
					}
					if(FLAGCRYPTO == CRYPTO_ETH)	{
						MAXLENGTHADDRESS = 20;		/*20 bytes beacuase we only need the data in binary*/
					}				
			break;
			//case MODE_CHECK:
			case MODE_MINIKEYS:
			case MODE_PUB2RMD:
			case MODE_RMD160:
				/* RMD160 marked as RAWDATA is a binary file and each register is 20 bytes	*/
				if(FLAGRAWDATA) {
					while(!feof(fd))	{
						if(fread(aux,1,20,fd) == 20)	{
							N++;
						}
					}
				}
				else	{
					while(!feof(fd))	{
						hextemp = fgets(aux,998,fd);
						if(hextemp == aux)	{
							trim(aux," \t\n\r");
							r = strlen(aux);
							if(r == 40)	{ //Any length for invalid Address?
								N++;
							}
						}
					}
				}
				MAXLENGTHADDRESS = 20;	/* MAXLENGTHADDRESS is 20 because we save the data in binary format */
			break;
			case MODE_XPOINT:
				if(FLAGRAWDATA) {
					while(!feof(fd))	{
						if(fread(aux,1,32,fd) == 32)	{
							N++;
						}
					}
				}
				else	{
					while(!feof(fd))	{
						hextemp = fgets(aux,998,fd);
						if(hextemp == aux)	{
							trim(aux," \t\n\r");
							r = strlen(aux);
							if(r >= 32)	{ //Any length for invalid Address?
								N++;
							}
						}
					}
				}
				MAXLENGTHADDRESS = 32;
			break;
		}
		free(aux);
		if(N == 0)	{
			fprintf(stderr,"[E] There is no valid data in the file\n");
			exit(0);
		}
		fseek(fd,0,SEEK_SET);

		printf("[+] Allocating memory for %" PRIu64 " elements: %.2f MB\n",N,(double)(((double) sizeof(struct address_value)*N)/(double)1048576));
		i = 0;
		addressTable = (struct address_value*) malloc(sizeof(struct address_value)*N);
		if(addressTable == NULL)	{
			fprintf(stderr,"[E] Can't alloc memory for %" PRIu64 " elements\n",N);
			exit(0);
		}
		printf("[+] Bloom filter for %" PRIu64 " elements.\n",N);
		if(N <= 1000)	{
			if(bloom_init2(&bloom,1000,0.000001)	== 1){
				fprintf(stderr,"[E] error bloom_init for 10000 elements.\n");
				exit(0);
			}
		}
		else	{
			if(bloom_init2(&bloom,N,0.000001)	== 1){
				fprintf(stderr,"[E] error bloom_init for %" PRIu64 " elements.\n",N);
				exit(0);
			}
		}
		printf("[+] Loading data to the bloomfilter total: %.2f MB\n",(double)(((double) bloom.bytes)/(double)1048576));
		i = 0;
		switch (FLAGMODE) {
			case MODE_ADDRESS:
				if(FLAGCRYPTO == CRYPTO_BTC)	{  // BTC address
					aux =(char*) malloc(2*MAXLENGTHADDRESS);
					if(aux == NULL)	{
						fprintf(stderr,"[E] error malloc()\n");
						exit(0);
					}
					while(i < N)	{
						memset(aux,0,2*MAXLENGTHADDRESS);
						memset(addressTable[i].value,0,sizeof(struct address_value));
						hextemp = fgets(aux,2*MAXLENGTHADDRESS,fd);
						if(hextemp == aux)	{
							trim(aux," \t\n\r");
							bloom_add(&bloom, aux,MAXLENGTHADDRESS);
							memcpy(addressTable[i].value,aux,20);
							i++;
						}
						else	{
							trim(aux," \t\n\r");
							fprintf(stderr,"[E] Omiting line : %s\n",aux);
						}
					}
				}
				if(FLAGCRYPTO == CRYPTO_ETH)	{  // ETH address
					aux =(char*) malloc(2*MAXLENGTHADDRESS + 4);	// 40 bytes + 0x and the charecter \0
					if(aux == NULL)	{
						fprintf(stderr,"[E] error malloc()\n");
						exit(0);
					}
					while(i < N)	{
						memset(addressTable[i].value,0,sizeof(struct address_value));
						memset(aux,0,2*MAXLENGTHADDRESS + 4);
						memset((void *)&addressTable[i],0,sizeof(struct address_value));
						hextemp = fgets(aux,2*MAXLENGTHADDRESS + 4,fd);
						if(hextemp == aux)	{
							trim(aux," \t\n\r");
							switch(strlen(aux))	{
								case 42:	/*Address with 0x */
									if(isValidHex(aux+2))	{
										hexs2bin(aux+2,addressTable[i].value);
										bloom_add(&bloom, addressTable[i].value,MAXLENGTHADDRESS);
									}
									else	{
										fprintf(stderr,"[E] Omiting line : %s\n",aux);
									}
								break;
								case 40:	/*Address without 0x */
									if(isValidHex(aux))	{
										hexs2bin(aux,addressTable[i].value);
										bloom_add(&bloom, addressTable[i].value,MAXLENGTHADDRESS);
									}
									else	{
										fprintf(stderr,"[E] Omiting line : %s\n",aux);
									}
								break;
								
							}
						}
						else	{
							trim(aux," \t\n\r");
							fprintf(stderr,"[E] Omiting line : %s\n",aux);
						}
						i++;
					}
					
				}
			break;
			case MODE_XPOINT:
				if(FLAGRAWDATA)	{
					aux = (char*)malloc(MAXLENGTHADDRESS);
					if(aux == NULL)	{
						fprintf(stderr,"[E] error malloc()\n");
						exit(0);
					}
					while(i < N)	{
						if(fread(aux,1,MAXLENGTHADDRESS,fd) == 32)	{
							memcpy(addressTable[i].value,aux,20);
							bloom_add(&bloom, aux,MAXLENGTHADDRESS);
						}
						i++;
					}
				}
				else	{
					aux = (char*) malloc(5*MAXLENGTHADDRESS);
					if(aux == NULL)	{
						fprintf(stderr,"[E] error malloc()\n");
						exit(0);
					}
					while(i < N)	{
						memset(aux,0,5*MAXLENGTHADDRESS);
						hextemp = fgets(aux,(5*MAXLENGTHADDRESS) -2,fd);
						memset((void *)&addressTable[i],0,sizeof(struct address_value));

						if(hextemp == aux)	{
							trim(aux," \t\n\r");
							stringtokenizer(aux,&tokenizer_xpoint);
							hextemp = nextToken(&tokenizer_xpoint);
							lenaux = strlen(hextemp);
							if(isValidHex(hextemp)) {
								switch(lenaux)	{
									case 64:	/*X value*/
										r = hexs2bin(aux,(uint8_t*) rawvalue);
										if(r)	{
											memcpy(addressTable[i].value,rawvalue,20);
											bloom_add(&bloom,rawvalue,MAXLENGTHADDRESS);
										}
										else	{
											fprintf(stderr,"[E] error hexs2bin\n");
										}
									break;
									case 66:	/*Compress publickey*/
									r = hexs2bin(aux+2, (uint8_t*)rawvalue);
										if(r)	{
											memcpy(addressTable[i].value,rawvalue,20);
											bloom_add(&bloom,rawvalue,MAXLENGTHADDRESS);
										}
										else	{
											fprintf(stderr,"[E] error hexs2bin\n");
										}
									break;
									case 130:	/* Uncompress publickey length*/
										memset(temporal,0,65);
										memcpy(temporal,aux+2,64);
										r = hexs2bin(temporal, (uint8_t*) rawvalue);
										if(r)	{
												memcpy(addressTable[i].value,rawvalue,20);
												bloom_add(&bloom,rawvalue,MAXLENGTHADDRESS);
										}
										else	{
											fprintf(stderr,"[E] error hexs2bin\n");
										}
									break;
									default:
										fprintf(stderr,"[E] Omiting line unknow length size %i: %s\n",lenaux,aux);
									break;
								}
							}
							else	{
								fprintf(stderr,"[E] Ignoring invalid hexvalue %s\n",aux);
							}
							freetokenizer(&tokenizer_xpoint);
						}
						else	{
							fprintf(stderr,"[E] Omiting line : %s\n",aux);
							N--;
						}
						i++;
					}
				}
			break;
			//case MODE_CHECK:
			case MODE_MINIKEYS:
			case MODE_PUB2RMD:
			case MODE_RMD160:
				if(FLAGRAWDATA)	{
					aux = (char*) malloc(MAXLENGTHADDRESS);
					if(aux == NULL)	{
						fprintf(stderr,"[E] error malloc()\n");
						exit(0);
					}
					while(i < N)	{
						if(fread(aux,1,MAXLENGTHADDRESS,fd) == 20)	{
							memcpy(addressTable[i].value,aux,20);
							bloom_add(&bloom, aux,MAXLENGTHADDRESS);
						}
						i++;
					}
				}
				else	{
					aux = (char*) malloc(3*MAXLENGTHADDRESS);
					if(aux == NULL)	{
						fprintf(stderr,"[E] error malloc()\n");
						exit(0);
					}
					while(i < N)	{
						memset(aux,0,3*MAXLENGTHADDRESS);
						hextemp = fgets(aux,3*MAXLENGTHADDRESS,fd);
						memset(addressTable[i].value,0,sizeof(struct address_value));
						if(hextemp == aux)	{
							trim(aux," \t\n\r");
							lenaux = strlen(aux);
							if(isValidHex(aux)) {
								if(lenaux == 40)	{
									if(hexs2bin(aux,addressTable[i].value))	{
											bloom_add(&bloom,addressTable[i].value,MAXLENGTHADDRESS);
									}
									else	{
										fprintf(stderr,"[E] error hexs2bin\n");
									}
								}
								else	{
									fprintf(stderr,"[E] Ignoring invalid length line %s\n",aux);
								}
							}
							else	{
								fprintf(stderr,"[E] Ignoring invalid hexvalue %s\n",aux);
							}
						}
						else	{
							fprintf(stderr,"[E] Omiting line : %s\n",aux);
						}
						i++;
					}
				}
			break;
		}
		free(aux);
		fclose(fd);
		printf("[+] Bloomfilter completed\n");
		if(FLAGALREADYSORTED)	{
			printf("[+] File mark already sorted, skipping sort proccess\n");
			printf("[+] %" PRIu64 " values were loaded\n",N);
			_sort(addressTable,N);
		}
		else	{
			printf("[+] Sorting data ...");
			_sort(addressTable,N);
			printf(" done! %" PRIu64 " values were loaded and sorted\n",N);
		}
	}
	if(FLAGMODE == MODE_BSGS )	{

		aux = (char*) malloc(1024);
		if(aux == NULL)	{
			fprintf(stderr,"[E] error malloc()\n");
			exit(0);
		}
		while(!feof(fd))	{
			if(fgets(aux,1022,fd) == aux)	{
				trim(aux," \t\n\r");
				if(strlen(aux) >= 128)	{	//Length of a full address in hexadecimal without 04
						N++;
				}else	{
					if(strlen(aux) >= 66)	{
						N++;
					}
				}
			}
		}
		if(N == 0)	{
			fprintf(stderr,"[E] There is no valid data in the file\n");
			exit(0);
		}
		bsgs_found = (int*) calloc(N,sizeof(int));
		OriginalPointsBSGS.reserve(N);
		OriginalPointsBSGScompressed = (bool*) malloc(N*sizeof(bool));
		pointx_str = (char*) malloc(65);
		pointy_str = (char*) malloc(65);
		if(pointy_str == NULL || pointx_str == NULL || bsgs_found == NULL)	{
			fprintf(stderr,"[E] error malloc()\n");
			exit(0);
		}
		fseek(fd,0,SEEK_SET);
		i = 0;
		while(!feof(fd))	{
			if(fgets(aux,1022,fd) == aux)	{
				trim(aux," \t\n\r");
				if(strlen(aux) >= 66)	{
					stringtokenizer(aux,&tokenizerbsgs);
					aux2 = nextToken(&tokenizerbsgs);
					memset(pointx_str,0,65);
					memset(pointy_str,0,65);
					switch(strlen(aux2))	{
						case 66:	//Compress

							if(secp->ParsePublicKeyHex(aux2,OriginalPointsBSGS[i],OriginalPointsBSGScompressed[i]))	{
								i++;
							}
							else	{
								N--;
							}

						break;
						case 130:	//With the 04

							if(secp->ParsePublicKeyHex(aux2,OriginalPointsBSGS[i],OriginalPointsBSGScompressed[i]))	{
								i++;
							}
							else	{
								N--;
							}

						break;
						default:
							printf("Invalid length: %s\n",aux2);
							N--;
						break;
					}
					freetokenizer(&tokenizerbsgs);
				}
			}
		}
		fclose(fd);
		bsgs_point_number = N;
		if(bsgs_point_number > 0)	{
			printf("[+] Added %u points from file\n",bsgs_point_number);
		}
		else	{
			printf("[E] The file don't have any valid publickeys\n");
			exit(0);
		}
		BSGS_N.SetInt32(0);
		BSGS_M.SetInt32(0);
		

		BSGS_M.SetInt64(bsgs_m);


		if(FLAG_N)	{	//Custom N by the -n param
						
			/* Here we need to validate if the given string is a valid hexadecimal number or a base 10 number*/
			
			/* Now the conversion*/
			if(str_N[0] == '0' && str_N[1] == 'x' )	{	/*We expected a hexadecimal value after 0x  -> str_N +2 */
				BSGS_N.SetBase16((char*)(str_N+2));
			}
			else	{
				BSGS_N.SetBase10(str_N);
			}
			
		}
		else	{	//Default N
			BSGS_N.SetInt64((uint64_t)0x100000000000);
		}

		if(BSGS_N.HasSqrt())	{	//If the root is exact
			BSGS_M.Set(&BSGS_N);
			BSGS_M.ModSqrt();
		}
		else	{
			fprintf(stderr,"[E] -n param doesn't have exact square root\n");
			exit(0);
		}

		BSGS_AUX.Set(&BSGS_M);
		BSGS_AUX.Mod(&BSGS_GROUP_SIZE);	
		
		if(!BSGS_AUX.IsZero()){ //If M is not divisible by  BSGS_GROUP_SIZE (1024) 
			hextemp = BSGS_GROUP_SIZE.GetBase10();
			fprintf(stderr,"[E] M value is not divisible by %s\n",hextemp);
			exit(0);
		}

		bsgs_m = BSGS_M.GetInt64();

		if(FLAGRANGE || FLAGBITRANGE)	{
			if(FLAGBITRANGE)	{	// Bit Range
				n_range_start.SetBase16(bit_range_str_min);
				n_range_end.SetBase16(bit_range_str_max);

				n_range_diff.Set(&n_range_end);
				n_range_diff.Sub(&n_range_start);
				printf("[+] Bit Range %i\n",bitrange);
				printf("[+] -- from : 0x%s\n",bit_range_str_min);
				printf("[+] -- to   : 0x%s\n",bit_range_str_max);
			}
			else	{
				printf("[+] Range \n");
				printf("[+] -- from : 0x%s\n",range_start);
				printf("[+] -- to   : 0x%s\n",range_end);
			}
		}
		else	{	//Random start

			n_range_start.SetInt32(1);
			n_range_end.Set(&secp->order);
			n_range_diff.Rand(&n_range_start,&n_range_end);
			n_range_start.Set(&n_range_diff);
		}
		BSGS_CURRENT.Set(&n_range_start);


		if(n_range_diff.IsLower(&BSGS_N) )	{
			fprintf(stderr,"[E] the given range is small\n");
			exit(0);
		}
		
		/*
	M	2199023255552
		109951162777.6
	M2	109951162778
		5497558138.9
	M3	5497558139
		*/

		BSGS_M.Mult((uint64_t)KFACTOR);
		BSGS_AUX.SetInt32(32);
		BSGS_R.Set(&BSGS_M);
		BSGS_R.Mod(&BSGS_AUX);
		BSGS_M2.Set(&BSGS_M);
		BSGS_M2.Div(&BSGS_AUX);

		if(!BSGS_R.IsZero())	{ /* If BSGS_M modulo 32 is not 0*/
			BSGS_M2.AddOne();
		}
		
		BSGS_R.Set(&BSGS_M2);
		BSGS_R.Mod(&BSGS_AUX);
		
		BSGS_M3.Set(&BSGS_M2);
		BSGS_M3.Div(&BSGS_AUX);
		
		if(!BSGS_R.IsZero())	{ /* If BSGS_M2 modulo 32 is not 0*/
			BSGS_M3.AddOne();
		}
		
		bsgs_m2 =  BSGS_M2.GetInt64();
		bsgs_m3 =  BSGS_M3.GetInt64();
		
		BSGS_AUX.Set(&BSGS_N);
		BSGS_AUX.Div(&BSGS_M);
		
		BSGS_R.Set(&BSGS_N);
		BSGS_R.Mod(&BSGS_M);

		if(!BSGS_R.IsZero())	{ /* if BSGS_N modulo BSGS_M is not 0*/
			BSGS_N.Set(&BSGS_M);
			BSGS_N.Mult(&BSGS_AUX);
		}

		bsgs_m = BSGS_M.GetInt64();
		bsgs_aux = BSGS_AUX.GetInt64();
		
		
		hextemp = BSGS_N.GetBase16();
		printf("[+] N = 0x%s\n",hextemp);
		free(hextemp);
		if(((uint64_t)(bsgs_m/256)) > 10000)	{
			itemsbloom = (uint64_t)(bsgs_m / 256);
			if(bsgs_m % 256 != 0 )	{
				itemsbloom++;
			}
		}
		else{
			itemsbloom = 1000;
		}
		
		if(((uint64_t)(bsgs_m2/256)) > 1000)	{
			itemsbloom2 = (uint64_t)(bsgs_m2 / 256);
			if(bsgs_m2 % 256 != 0)	{
				itemsbloom2++;
			}
		}
		else	{
			itemsbloom2 = 1000;
		}
		
		if(((uint64_t)(bsgs_m3/256)) > 1000)	{
			itemsbloom3 = (uint64_t)(bsgs_m3/256);
			if(bsgs_m3 % 256 != 0 )	{
				itemsbloom3++;
			}
		}
		else	{
			itemsbloom3 = 1000;
		}
		
		printf("[+] Bloom filter for %" PRIu64 " elements ",bsgs_m);
		bloom_bP = (struct bloom*)calloc(256,sizeof(struct bloom));
		bloom_bP_checksums = (struct checksumsha256*)calloc(256,sizeof(struct checksumsha256));
		
#if defined(_WIN64) && !defined(__CYGWIN__)
		bloom_bP_mutex = (HANDLE*) calloc(256,sizeof(HANDLE));
#else
		bloom_bP_mutex = (pthread_mutex_t*) calloc(256,sizeof(pthread_mutex_t));
#endif
		
		if(bloom_bP == NULL || bloom_bP_checksums == NULL || bloom_bP_mutex == NULL )	{
			fprintf(stderr,"[E] error calloc()\n");
			exit(0);
		}
		fflush(stdout);
		bloom_bP_totalbytes = 0;
		for(i=0; i< 256; i++)	{
#if defined(_WIN64) && !defined(__CYGWIN__)
			bloom_bP_mutex[i] = CreateMutex(NULL, FALSE, NULL);
#else
			pthread_mutex_init(&bloom_bP_mutex[i],NULL);
#endif
			if(bloom_init2(&bloom_bP[i],itemsbloom,0.000001)	== 1){
				fprintf(stderr,"[E] error bloom_init _ [%" PRIu64 "]\n",i);
				exit(0);
			}
			bloom_bP_totalbytes += bloom_bP[i].bytes;
			//if(FLAGDEBUG) bloom_print(&bloom_bP[i]);
		}
		printf(": %.2f MB\n",(float)((float)(uint64_t)bloom_bP_totalbytes/(float)(uint64_t)1048576));


		printf("[+] Bloom filter for %" PRIu64 " elements ",bsgs_m2);
		
#if defined(_WIN64) && !defined(__CYGWIN__)
		bloom_bPx2nd_mutex = (HANDLE*) calloc(256,sizeof(HANDLE));
#else
		bloom_bPx2nd_mutex = (pthread_mutex_t*) calloc(256,sizeof(pthread_mutex_t));
#endif
		bloom_bPx2nd = (struct bloom*)calloc(256,sizeof(struct bloom));
		bloom_bPx2nd_checksums = (struct checksumsha256*) calloc(256,sizeof(struct checksumsha256));

		if(bloom_bPx2nd == NULL || bloom_bPx2nd_checksums == NULL || bloom_bPx2nd_mutex == NULL )	{
			fprintf(stderr,"[E] error calloc()\n");
		}
		bloom_bP2_totalbytes = 0;
		for(i=0; i< 256; i++)	{
#if defined(_WIN64) && !defined(__CYGWIN__)
			bloom_bPx2nd_mutex[i] = CreateMutex(NULL, FALSE, NULL);
#else
			pthread_mutex_init(&bloom_bPx2nd_mutex[i],NULL);
#endif
			if(bloom_init2(&bloom_bPx2nd[i],itemsbloom2,0.000001)	== 1){
				fprintf(stderr,"[E] error bloom_init _ [%" PRIu64 "]\n",i);
				exit(0);
			}
			bloom_bP2_totalbytes += bloom_bPx2nd[i].bytes;
			//if(FLAGDEBUG) bloom_print(&bloom_bPx2nd[i]);
		}
		printf(": %.2f MB\n",(float)((float)(uint64_t)bloom_bP2_totalbytes/(float)(uint64_t)1048576));
		

#if defined(_WIN64) && !defined(__CYGWIN__)
		bloom_bPx3rd_mutex = (HANDLE*) calloc(256,sizeof(HANDLE));
#else
		bloom_bPx3rd_mutex = (pthread_mutex_t*) calloc(256,sizeof(pthread_mutex_t));
#endif
	
		bloom_bPx3rd = (struct bloom*)calloc(256,sizeof(struct bloom));
		bloom_bPx3rd_checksums = (struct checksumsha256*) calloc(256,sizeof(struct checksumsha256));

		if(bloom_bPx3rd == NULL || bloom_bPx3rd_checksums == NULL || bloom_bPx3rd_mutex == NULL )	{
			fprintf(stderr,"[E] error calloc()\n");
		}

		
		printf("[+] Bloom filter for %" PRIu64 " elements ",bsgs_m3);
		bloom_bP3_totalbytes = 0;
		for(i=0; i< 256; i++)	{
#if defined(_WIN64) && !defined(__CYGWIN__)
			bloom_bPx3rd_mutex[i] = CreateMutex(NULL, FALSE, NULL);
#else
			pthread_mutex_init(&bloom_bPx3rd_mutex[i],NULL);
#endif
			if(bloom_init2(&bloom_bPx3rd[i],itemsbloom3,0.000001)	== 1){
				fprintf(stderr,"[E] error bloom_init [%" PRIu64 "]\n",i);
				exit(0);
			}
			bloom_bP3_totalbytes += bloom_bPx3rd[i].bytes;
			//if(FLAGDEBUG) bloom_print(&bloom_bPx3rd[i]);
		}
		printf(": %.2f MB\n",(float)((float)(uint64_t)bloom_bP3_totalbytes/(float)(uint64_t)1048576));
		//if(FLAGDEBUG) printf("[D] bloom_bP3_totalbytes : %" PRIu64 "\n",bloom_bP3_totalbytes);




		BSGS_MP = secp->ComputePublicKey(&BSGS_M);
		BSGS_MP2 = secp->ComputePublicKey(&BSGS_M2);
		BSGS_MP3 = secp->ComputePublicKey(&BSGS_M3);
		
		BSGS_AMP2.reserve(32);
		BSGS_AMP3.reserve(32);
		GSn.reserve(CPU_GRP_SIZE/2);
		GSn2.reserve(16);
		GSn3.reserve(16);

		i= 0;


		/* New aMP table just to keep the same code of JLP */
		/* Auxiliar Points to speed up calculations for the main bloom filter check */
		Point bsP = secp->Negation(BSGS_MP);
		Point g = bsP;
		GSn[0] = g;
		
		g = secp->DoubleDirect(g);
		GSn[1] = g;
		for(int i = 2; i < CPU_GRP_SIZE / 2; i++) {
			g = secp->AddDirect(g,bsP);
			GSn[i] = g;

		}
		_2GSn = secp->DoubleDirect(GSn[CPU_GRP_SIZE / 2 - 1]);
		
		
		/*Auxiliar Points to speed up calculations for the second bloom filter check */
		bsP = secp->Negation(BSGS_MP2);
		g = bsP;
		GSn2[0] = g;
		g = secp->DoubleDirect(g);
		GSn2[1] = g;
		for(int i = 2; i < 16; i++) {
			g = secp->AddDirect(g,bsP);
			GSn2[i] = g;

		}
		_2GSn2 = secp->DoubleDirect(GSn2[16 - 1]);
		
		/*Auxiliar Points to speed up calculations for the third bloom filter check */
		bsP = secp->Negation(BSGS_MP3);
		g = bsP;
		GSn3[0] = g;
		g = secp->DoubleDirect(g);
		GSn3[1] = g;
		for(int i = 2; i < 16; i++) {
			g = secp->AddDirect(g,bsP);
			GSn3[i] = g;

		}
		_2GSn3 = secp->DoubleDirect(GSn3[16 - 1]);
		
		
		
		
		
		point_temp.Set(BSGS_MP2);
		BSGS_AMP2[0] = secp->Negation(point_temp);
		point_temp = secp->DoubleDirect(BSGS_MP2);
		
		for(i = 1; i < 32; i++)	{
			BSGS_AMP2[i] = secp->Negation(point_temp);
			point_temp2 = secp->AddDirect(point_temp,BSGS_MP2);
			point_temp.Set(point_temp2);
		}
		
		point_temp.Set(BSGS_MP3);
		BSGS_AMP3[0] = secp->Negation(point_temp);
		point_temp = secp->DoubleDirect(BSGS_MP3);
		
		for(i = 1; i < 32; i++)	{
			BSGS_AMP3[i] = secp->Negation(point_temp);
			point_temp2 = secp->AddDirect(point_temp,BSGS_MP3);
			point_temp.Set(point_temp2);
		}

		bytes = (uint64_t)bsgs_m3 * (uint64_t) sizeof(struct bsgs_xvalue);
		printf("[+] Allocating %.2f MB for %" PRIu64  " bP Points\n",(double)(bytes/1048576),bsgs_m3);
		
		bPtable = (struct bsgs_xvalue*) malloc(bytes);
		if(bPtable == NULL)	{
			printf("[E] error malloc()\n");
			exit(0);
		}
		memset(bPtable,0,bytes);
		
		if(FLAGSAVEREADFILE)	{
			/*Reading file for 1st bloom filter */

			snprintf(buffer_bloom_file,1024,"keyhunt_bsgs_4_%" PRIu64 ".blm",bsgs_m);
			fd_aux1 = fopen(buffer_bloom_file,"rb");
			if(fd_aux1 != NULL)	{
				printf("[+] Reading bloom filter from file %s ",buffer_bloom_file);
				fflush(stdout);
				for(i = 0; i < 256;i++)	{
					bf_ptr = (char*) bloom_bP[i].bf;	/*We need to save the current bf pointer*/
					readed = fread(&bloom_bP[i],sizeof(struct bloom),1,fd_aux1);
					if(readed != 1)	{
						fprintf(stderr,"[E] Error reading the file %s\n",buffer_bloom_file);
						exit(0);
					}
					bloom_bP[i].bf = (uint8_t*)bf_ptr;	/* Restoring the bf pointer*/
					readed = fread(bloom_bP[i].bf,bloom_bP[i].bytes,1,fd_aux1);
					if(readed != 1)	{
						fprintf(stderr,"[E] Error reading the file %s\n",buffer_bloom_file);
						exit(0);
					}
					readed = fread(&bloom_bP_checksums[i],sizeof(struct checksumsha256),1,fd_aux1);
					if(readed != 1)	{
						fprintf(stderr,"[E] Error reading the file %s\n",buffer_bloom_file);
						exit(0);
					}
					memset(rawvalue,0,32);
					sha256((uint8_t*)bloom_bP[i].bf,bloom_bP[i].bytes,(uint8_t*)rawvalue);
					if(memcmp(bloom_bP_checksums[i].data,rawvalue,32) != 0 || memcmp(bloom_bP_checksums[i].backup,rawvalue,32) != 0 )	{	/* Verification */
						fprintf(stderr,"[E] Error checksum file mismatch!\n");
						exit(0);
					}
					/*
					if(FLAGDEBUG)	{
						hextemp = tohex(bloom_bP_checksums[i].data,32);
						printf("Checksum %s\n",hextemp);
						free(hextemp);
						bloom_print(&bloom_bP[i]);
					}
					*/
					if(i % 64 == 0 )	{
						printf(".");
						fflush(stdout);
					}
				}
				printf(" Done!\n");
				fclose(fd_aux1);
				memset(buffer_bloom_file,0,1024);
				snprintf(buffer_bloom_file,1024,"keyhunt_bsgs_3_%" PRIu64 ".blm",bsgs_m);
				fd_aux1 = fopen(buffer_bloom_file,"rb");
				if(fd_aux1 != NULL)	{
					printf("[W] Unused file detected %s you can delete it without worry\n",buffer_bloom_file);
					fclose(fd_aux1);
				}
				FLAGREADEDFILE1 = 1;
			}
			else	{	/*Checking for old file    keyhunt_bsgs_3_   */
				snprintf(buffer_bloom_file,1024,"keyhunt_bsgs_3_%" PRIu64 ".blm",bsgs_m);
				fd_aux1 = fopen(buffer_bloom_file,"rb");
				if(fd_aux1 != NULL)	{
					printf("[+] Reading bloom filter from file %s ",buffer_bloom_file);
					fflush(stdout);
					for(i = 0; i < 256;i++)	{
						bf_ptr = (char*) bloom_bP[i].bf;	/*We need to save the current bf pointer*/
						readed = fread(&oldbloom_bP,sizeof(struct oldbloom),1,fd_aux1);
						/*
						if(FLAGDEBUG)	{
							printf("old Bloom filter %i\n",i);
							oldbloom_print(&oldbloom_bP);
						}
						*/
						
						if(readed != 1)	{
							fprintf(stderr,"[E] Error reading the file %s\n",buffer_bloom_file);
							exit(0);
						}
						memcpy(&bloom_bP[i],&oldbloom_bP,sizeof(struct bloom));//We only need to copy the part data to the new bloom size, not from the old size
						bloom_bP[i].bf = (uint8_t*)bf_ptr;	/* Restoring the bf pointer*/
						
						readed = fread(bloom_bP[i].bf,bloom_bP[i].bytes,1,fd_aux1);
						if(readed != 1)	{
							fprintf(stderr,"[E] Error reading the file %s\n",buffer_bloom_file);
							exit(0);
						}
						memcpy(bloom_bP_checksums[i].data,oldbloom_bP.checksum,32);
						memcpy(bloom_bP_checksums[i].backup,oldbloom_bP.checksum_backup,32);
						memset(rawvalue,0,32);
						sha256((uint8_t*)bloom_bP[i].bf,bloom_bP[i].bytes,(uint8_t*)rawvalue);
						if(memcmp(bloom_bP_checksums[i].data,rawvalue,32) != 0 || memcmp(bloom_bP_checksums[i].backup,rawvalue,32) != 0 )	{	/* Verification */
							fprintf(stderr,"[E] Error checksum file mismatch!\n");
							exit(0);
						}
						if(i % 32 == 0 )	{
							printf(".");
							fflush(stdout);
						}
					}
					printf(" Done!\n");
					fclose(fd_aux1);
					FLAGUPDATEFILE1 = 1;	/* Flag to migrate the data to the new File keyhunt_bsgs_4_ */
					FLAGREADEDFILE1 = 1;
					
				}
				else	{
					FLAGREADEDFILE1 = 0;
					//Flag to make the new file
				}
			}
			
			/*Reading file for 2nd bloom filter */
			snprintf(buffer_bloom_file,1024,"keyhunt_bsgs_6_%" PRIu64 ".blm",bsgs_m2);
			fd_aux2 = fopen(buffer_bloom_file,"rb");
			if(fd_aux2 != NULL)	{
				printf("[+] Reading bloom filter from file %s ",buffer_bloom_file);
				fflush(stdout);
				for(i = 0; i < 256;i++)	{
					bf_ptr = (char*) bloom_bPx2nd[i].bf;	/*We need to save the current bf pointer*/
					readed = fread(&bloom_bPx2nd[i],sizeof(struct bloom),1,fd_aux2);
					if(readed != 1)	{
						fprintf(stderr,"[E] Error reading the file %s\n",buffer_bloom_file);
						exit(0);
					}
					bloom_bPx2nd[i].bf = (uint8_t*)bf_ptr;	/* Restoring the bf pointer*/
					readed = fread(bloom_bPx2nd[i].bf,bloom_bPx2nd[i].bytes,1,fd_aux2);
					if(readed != 1)	{
						fprintf(stderr,"[E] Error reading the file %s\n",buffer_bloom_file);
						exit(0);
					}
					readed = fread(&bloom_bPx2nd_checksums[i],sizeof(struct checksumsha256),1,fd_aux2);
					if(readed != 1)	{
						fprintf(stderr,"[E] Error reading the file %s\n",buffer_bloom_file);
						exit(0);
					}
					memset(rawvalue,0,32);
					sha256((uint8_t*)bloom_bPx2nd[i].bf,bloom_bPx2nd[i].bytes,(uint8_t*)rawvalue);
					if(memcmp(bloom_bPx2nd_checksums[i].data,rawvalue,32) != 0 || memcmp(bloom_bPx2nd_checksums[i].backup,rawvalue,32) != 0 )	{		/* Verification */
						fprintf(stderr,"[E] Error checksum file mismatch!\n");
						exit(0);
					}
					if(i % 64 == 0)	{
						printf(".");
						fflush(stdout);
					}

					/*
					if(FLAGDEBUG)	{
						hextemp = tohex(bloom_bPx2nd_checksum[i].data,32);
						printf("Checksum %s\n",hextemp);
						free(hextemp);
						bloom_print(&bloom_bPx2nd[i]);
					}
					*/
				}
				fclose(fd_aux2);
				printf(" Done!\n");
				memset(buffer_bloom_file,0,1024);
				snprintf(buffer_bloom_file,1024,"keyhunt_bsgs_5_%" PRIu64 ".blm",bsgs_m2);
				fd_aux2 = fopen(buffer_bloom_file,"rb");
				if(fd_aux2 != NULL)	{
					printf("[W] Unused file detected %s you can delete it without worry\n",buffer_bloom_file);
					fclose(fd_aux2);
				}
				memset(buffer_bloom_file,0,1024);
				snprintf(buffer_bloom_file,1024,"keyhunt_bsgs_1_%" PRIu64 ".blm",bsgs_m2);
				fd_aux2 = fopen(buffer_bloom_file,"rb");
				if(fd_aux2 != NULL)	{
					printf("[W] Unused file detected %s you can delete it without worry\n",buffer_bloom_file);
					fclose(fd_aux2);
				}
				FLAGREADEDFILE2 = 1;
			}
			else	{	
				FLAGREADEDFILE2 = 0;
			}
			
			/*Reading file for bPtable */
			snprintf(buffer_bloom_file,1024,"keyhunt_bsgs_2_%" PRIu64 ".tbl",bsgs_m3);
			fd_aux3 = fopen(buffer_bloom_file,"rb");
			if(fd_aux3 != NULL)	{
				printf("[+] Reading bP Table from file %s .",buffer_bloom_file);
				fflush(stdout);
				fread(bPtable,bytes,1,fd_aux3);
				if(readed != 1)	{
					fprintf(stderr,"[E] Error reading the file %s\n",buffer_bloom_file);
					exit(0);
				}
				fread(checksum,32,1,fd_aux3);
				sha256((uint8_t*)bPtable,bytes,(uint8_t*)checksum_backup);
				if(memcmp(checksum,checksum_backup,32) != 0)	{
					fprintf(stderr,"[E] Checksum from file %s mismatch!!\n",buffer_bloom_file);
					exit(0);
				}
				printf("... Done!\n");
				fclose(fd_aux3);
				FLAGREADEDFILE3 = 1;
			}
			else	{
				FLAGREADEDFILE3 = 0;
			}
			
			/*Reading file for 3rd bloom filter */
			snprintf(buffer_bloom_file,1024,"keyhunt_bsgs_7_%" PRIu64 ".blm",bsgs_m3);
			fd_aux2 = fopen(buffer_bloom_file,"rb");
			if(fd_aux2 != NULL)	{
				printf("[+] Reading bloom filter from file %s ",buffer_bloom_file);
				fflush(stdout);
				for(i = 0; i < 256;i++)	{
					bf_ptr = (char*) bloom_bPx3rd[i].bf;	/*We need to save the current bf pointer*/
					readed = fread(&bloom_bPx3rd[i],sizeof(struct bloom),1,fd_aux2);
					if(readed != 1)	{
						fprintf(stderr,"[E] Error reading the file %s\n",buffer_bloom_file);
						exit(0);
					}
					bloom_bPx3rd[i].bf = (uint8_t*)bf_ptr;	/* Restoring the bf pointer*/
					readed = fread(bloom_bPx3rd[i].bf,bloom_bPx3rd[i].bytes,1,fd_aux2);
					if(readed != 1)	{
						fprintf(stderr,"[E] Error reading the file %s\n",buffer_bloom_file);
						exit(0);
					}
					readed = fread(&bloom_bPx3rd_checksums[i],sizeof(struct checksumsha256),1,fd_aux2);
					if(readed != 1)	{
						fprintf(stderr,"[E] Error reading the file %s\n",buffer_bloom_file);
						exit(0);
					}
					memset(rawvalue,0,32);
					sha256((uint8_t*)bloom_bPx3rd[i].bf,bloom_bPx3rd[i].bytes,(uint8_t*)rawvalue);
					if(memcmp(bloom_bPx3rd_checksums[i].data,rawvalue,32) != 0 || memcmp(bloom_bPx3rd_checksums[i].backup,rawvalue,32) != 0 )	{		/* Verification */
						fprintf(stderr,"[E] Error checksum file mismatch!\n");
						exit(0);
					}
					if(i % 64 == 0)	{
						printf(".");
						fflush(stdout);
					}
				}
				fclose(fd_aux2);
				printf(" Done!\n");
				FLAGREADEDFILE4 = 1;
			}
			else	{
				FLAGREADEDFILE4 = 0;
			}
			
		}
		
		if(!FLAGREADEDFILE1 || !FLAGREADEDFILE2 || !FLAGREADEDFILE3 || !FLAGREADEDFILE4)	{
			if(FLAGREADEDFILE1 == 1)	{
				/* 
					We need just to make File 2 to File 4 this is
					- Second bloom filter 5%
					- third  bloom fitler 0.25 %
					- bp Table 0.25 %
				*/
				printf("[I] We need to recalculate some files, don't worry this is only 3%% of the previous work\n");
				FINISHED_THREADS_COUNTER = 0;
				FINISHED_THREADS_BP = 0;
				FINISHED_ITEMS = 0;
				salir = 0;
				BASE = 0;
				THREADCOUNTER = 0;
				if(THREADBPWORKLOAD >= bsgs_m2)	{
					THREADBPWORKLOAD = bsgs_m2;
				}
				THREADCYCLES = bsgs_m2 / THREADBPWORKLOAD;
				PERTHREAD_R = bsgs_m2 % THREADBPWORKLOAD;
				if(FLAGDEBUG) printf("[D] THREADCYCLES: %lu\n",THREADCYCLES);
				if(PERTHREAD_R != 0)	{
					THREADCYCLES++;
					if(FLAGDEBUG) printf("[D] PERTHREAD_R: %lu\n",PERTHREAD_R);
				}
				
				printf("\r[+] processing %lu/%lu bP points : %i%%\r",FINISHED_ITEMS,bsgs_m,(int) (((double)FINISHED_ITEMS/(double)bsgs_m)*100));
				fflush(stdout);
				
#if defined(_WIN64) && !defined(__CYGWIN__)
				tid = (HANDLE*)calloc(NTHREADS, sizeof(HANDLE));
				bPload_mutex = (HANDLE*) calloc(NTHREADS,sizeof(HANDLE));
#else
				tid = (pthread_t *) calloc(NTHREADS,sizeof(pthread_t));
				bPload_mutex = (pthread_mutex_t*) calloc(NTHREADS,sizeof(pthread_mutex_t));
#endif
				bPload_temp_ptr = (struct bPload*) calloc(NTHREADS,sizeof(struct bPload));
				bPload_threads_available = (char*) calloc(NTHREADS,sizeof(char));
				
				if(tid == NULL || bPload_temp_ptr == NULL || bPload_threads_available == NULL || bPload_mutex == NULL)	{
					fprintf(stderr,"[E] error calloc()\n");
					exit(0);
				}
				memset(bPload_threads_available,1,NTHREADS);
				
				for(i = 0; i < NTHREADS; i++)	{
#if defined(_WIN64) && !defined(__CYGWIN__)
					bPload_mutex[i] = CreateMutex(NULL, FALSE, NULL);
#else
					pthread_mutex_init(&bPload_mutex[i],NULL);
#endif
				}
				
				do	{
					for(i = 0; i < NTHREADS && !salir; i++)	{

						if(bPload_threads_available[i] && !salir)	{
							bPload_threads_available[i] = 0;
							bPload_temp_ptr[i].from = BASE;
							bPload_temp_ptr[i].threadid = i;
							bPload_temp_ptr[i].finished = 0;
							if( THREADCOUNTER < THREADCYCLES-1)	{
								bPload_temp_ptr[i].to = BASE + THREADBPWORKLOAD;
								bPload_temp_ptr[i].workload = THREADBPWORKLOAD;
							}
							else	{
								bPload_temp_ptr[i].to = BASE + THREADBPWORKLOAD + PERTHREAD_R;
								bPload_temp_ptr[i].workload = THREADBPWORKLOAD + PERTHREAD_R;
								salir = 1;
								//if(FLAGDEBUG) printf("[D] Salir OK\n");
							}
							//if(FLAGDEBUG) printf("[I] %lu to %lu\n",bPload_temp_ptr[i].from,bPload_temp_ptr[i].to);
#if defined(_WIN64) && !defined(__CYGWIN__)
							tid[i] = CreateThread(NULL, 0, thread_bPload_2blooms, (void*) &bPload_temp_ptr[i], 0, &s);
#else
							s = pthread_create(&tid[i],NULL,thread_bPload_2blooms,(void*) &bPload_temp_ptr[i]);
							pthread_detach(tid[i]);
#endif
							BASE+=THREADBPWORKLOAD;
							THREADCOUNTER++;
						}
					}

					if(OLDFINISHED_ITEMS != FINISHED_ITEMS)	{
						printf("\r[+] processing %lu/%lu bP points : %i%%\r",FINISHED_ITEMS,bsgs_m2,(int) (((double)FINISHED_ITEMS/(double)bsgs_m2)*100));
						fflush(stdout);
						OLDFINISHED_ITEMS = FINISHED_ITEMS;
					}
					
					for(i = 0 ; i < NTHREADS ; i++)	{

#if defined(_WIN64) && !defined(__CYGWIN__)
						WaitForSingleObject(bPload_mutex[i], INFINITE);
						finished = bPload_temp_ptr[i].finished;
						ReleaseMutex(bPload_mutex[i]);
#else
						pthread_mutex_lock(&bPload_mutex[i]);
						finished = bPload_temp_ptr[i].finished;
						pthread_mutex_unlock(&bPload_mutex[i]);
#endif
						if(finished)	{
							bPload_temp_ptr[i].finished = 0;
							bPload_threads_available[i] = 1;
							FINISHED_ITEMS += bPload_temp_ptr[i].workload;
							FINISHED_THREADS_COUNTER++;
						}
					}
					
				}while(FINISHED_THREADS_COUNTER < THREADCYCLES);
				printf("\r[+] processing %lu/%lu bP points : 100%%     \n",bsgs_m2,bsgs_m2);
				
				free(tid);
				free(bPload_mutex);
				free(bPload_temp_ptr);
				free(bPload_threads_available);
			}
			else{	
				/* We need just to do all the files 
					- first  bllom filter 100% 
					- Second bloom filter 5%
					- third  bloom fitler 0.25 %
					- bp Table 0.25 %
				*/
				FINISHED_THREADS_COUNTER = 0;
				FINISHED_THREADS_BP = 0;
				FINISHED_ITEMS = 0;
				salir = 0;
				BASE = 0;
				THREADCOUNTER = 0;
				if(THREADBPWORKLOAD >= bsgs_m)	{
					THREADBPWORKLOAD = bsgs_m;
				}
				THREADCYCLES = bsgs_m / THREADBPWORKLOAD;
				PERTHREAD_R = bsgs_m % THREADBPWORKLOAD;
				if(FLAGDEBUG) printf("[D] THREADCYCLES: %lu\n",THREADCYCLES);
				if(PERTHREAD_R != 0)	{
					THREADCYCLES++;
					if(FLAGDEBUG) printf("[D] PERTHREAD_R: %lu\n",PERTHREAD_R);
				}
				
				printf("\r[+] processing %lu/%lu bP points : %i%%\r",FINISHED_ITEMS,bsgs_m,(int) (((double)FINISHED_ITEMS/(double)bsgs_m)*100));
				fflush(stdout);
				
#if defined(_WIN64) && !defined(__CYGWIN__)
				tid = (HANDLE*)calloc(NTHREADS, sizeof(HANDLE));
				bPload_mutex = (HANDLE*) calloc(NTHREADS,sizeof(HANDLE));
#else
				tid = (pthread_t *) calloc(NTHREADS,sizeof(pthread_t));
				bPload_mutex = (pthread_mutex_t*) calloc(NTHREADS,sizeof(pthread_mutex_t));
#endif
				bPload_temp_ptr = (struct bPload*) calloc(NTHREADS,sizeof(struct bPload));
				bPload_threads_available = (char*) calloc(NTHREADS,sizeof(char));
				
				if(tid == NULL || bPload_temp_ptr == NULL || bPload_threads_available == NULL || bPload_mutex == NULL)	{
					fprintf(stderr,"[E] error calloc()\n");
					exit(0);
				}
				memset(bPload_threads_available,1,NTHREADS);
				
				for(i = 0; i < NTHREADS; i++)	{
#if defined(_WIN64) && !defined(__CYGWIN__)
					bPload_mutex = CreateMutex(NULL, FALSE, NULL);
#else
					pthread_mutex_init(&bPload_mutex[i],NULL);
#endif
				}
				
				do	{
					for(i = 0; i < NTHREADS && !salir; i++)	{

						if(bPload_threads_available[i] && !salir)	{
							bPload_threads_available[i] = 0;
							bPload_temp_ptr[i].from = BASE;
							bPload_temp_ptr[i].threadid = i;
							bPload_temp_ptr[i].finished = 0;
							if( THREADCOUNTER < THREADCYCLES-1)	{
								bPload_temp_ptr[i].to = BASE + THREADBPWORKLOAD;
								bPload_temp_ptr[i].workload = THREADBPWORKLOAD;
							}
							else	{
								bPload_temp_ptr[i].to = BASE + THREADBPWORKLOAD + PERTHREAD_R;
								bPload_temp_ptr[i].workload = THREADBPWORKLOAD + PERTHREAD_R;
								salir = 1;
								//if(FLAGDEBUG) printf("[D] Salir OK\n");
							}
							//if(FLAGDEBUG) printf("[I] %lu to %lu\n",bPload_temp_ptr[i].from,bPload_temp_ptr[i].to);
#if defined(_WIN64) && !defined(__CYGWIN__)
							tid[i] = CreateThread(NULL, 0, thread_bPload, (void*) &bPload_temp_ptr[i], 0, &s);
#else
							s = pthread_create(&tid[i],NULL,thread_bPload,(void*) &bPload_temp_ptr[i]);
							pthread_detach(tid[i]);
#endif
							BASE+=THREADBPWORKLOAD;
							THREADCOUNTER++;
						}
					}
					if(OLDFINISHED_ITEMS != FINISHED_ITEMS)	{
						printf("\r[+] processing %lu/%lu bP points : %i%%\r",FINISHED_ITEMS,bsgs_m,(int) (((double)FINISHED_ITEMS/(double)bsgs_m)*100));
						fflush(stdout);
						OLDFINISHED_ITEMS = FINISHED_ITEMS;
					}
					
					for(i = 0 ; i < NTHREADS ; i++)	{

#if defined(_WIN64) && !defined(__CYGWIN__)
						WaitForSingleObject(bPload_mutex[i], INFINITE);
						finished = bPload_temp_ptr[i].finished;
						ReleaseMutex(bPload_mutex[i]);
#else
						pthread_mutex_lock(&bPload_mutex[i]);
						finished = bPload_temp_ptr[i].finished;
						pthread_mutex_unlock(&bPload_mutex[i]);
#endif
						if(finished)	{
							bPload_temp_ptr[i].finished = 0;
							bPload_threads_available[i] = 1;
							FINISHED_ITEMS += bPload_temp_ptr[i].workload;
							FINISHED_THREADS_COUNTER++;
						}
					}
					
				}while(FINISHED_THREADS_COUNTER < THREADCYCLES);
				printf("\r[+] processing %lu/%lu bP points : 100%%     \n",bsgs_m,bsgs_m);
				
				free(tid);
				free(bPload_mutex);
				free(bPload_temp_ptr);
				free(bPload_threads_available);
			}
		}
		
		if(!FLAGREADEDFILE1 || !FLAGREADEDFILE2 || !FLAGREADEDFILE4)	{
			printf("[+] Making checkums .. ");
			fflush(stdout);
		}	
		if(!FLAGREADEDFILE1)	{
			for(i = 0; i < 256 ; i++)	{
				sha256((uint8_t*)bloom_bP[i].bf, bloom_bP[i].bytes,(uint8_t*) bloom_bP_checksums[i].data);
				memcpy(bloom_bP_checksums[i].backup,bloom_bP_checksums[i].data,32);
			}
			printf(".");
		}
		if(!FLAGREADEDFILE2)	{
			for(i = 0; i < 256 ; i++)	{
				sha256((uint8_t*)bloom_bPx2nd[i].bf, bloom_bPx2nd[i].bytes,(uint8_t*) bloom_bPx2nd_checksums[i].data);
				memcpy(bloom_bPx2nd_checksums[i].backup,bloom_bPx2nd_checksums[i].data,32);
			}
			printf(".");
		}
		if(!FLAGREADEDFILE4)	{
			for(i = 0; i < 256 ; i++)	{
				sha256((uint8_t*)bloom_bPx3rd[i].bf, bloom_bPx3rd[i].bytes,(uint8_t*) bloom_bPx3rd_checksums[i].data);
				memcpy(bloom_bPx3rd_checksums[i].backup,bloom_bPx3rd_checksums[i].data,32);
			}
			printf(".");
		}
		if(!FLAGREADEDFILE1 || !FLAGREADEDFILE2 || !FLAGREADEDFILE4)	{
			printf(" done\n");
			fflush(stdout);
		}	
		if(!FLAGREADEDFILE3)	{
			printf("[+] Sorting %lu elements... ",bsgs_m3);
			fflush(stdout);
			bsgs_sort(bPtable,bsgs_m3);
			sha256((uint8_t*)bPtable, bytes,(uint8_t*) checksum);
			memcpy(checksum_backup,checksum,32);
			printf("Done!\n");
			fflush(stdout);
		}
		if(FLAGSAVEREADFILE || FLAGUPDATEFILE1 )	{
			if(!FLAGREADEDFILE1 || FLAGUPDATEFILE1)	{
				snprintf(buffer_bloom_file,1024,"keyhunt_bsgs_4_%" PRIu64 ".blm",bsgs_m);
				
				if(FLAGUPDATEFILE1)	{
					printf("[W] Updating old file into a new one\n");
				}
				
				/* Writing file for 1st bloom filter */
				
				fd_aux1 = fopen(buffer_bloom_file,"wb");
				if(fd_aux1 != NULL)	{
					printf("[+] Writing bloom filter to file %s ",buffer_bloom_file);
					fflush(stdout);
					for(i = 0; i < 256;i++)	{
						readed = fwrite(&bloom_bP[i],sizeof(struct bloom),1,fd_aux1);
						if(readed != 1)	{
							fprintf(stderr,"[E] Error writing the file %s please delete it\n",buffer_bloom_file);
							exit(0);
						}
						readed = fwrite(bloom_bP[i].bf,bloom_bP[i].bytes,1,fd_aux1);
						if(readed != 1)	{
							fprintf(stderr,"[E] Error writing the file %s please delete it\n",buffer_bloom_file);
							exit(0);
						}
						readed = fwrite(&bloom_bP_checksums[i],sizeof(struct checksumsha256),1,fd_aux1);
						if(readed != 1)	{
							fprintf(stderr,"[E] Error writing the file %s please delete it\n",buffer_bloom_file);
							exit(0);
						}
						if(i % 64 == 0)	{
							printf(".");
							fflush(stdout);
						}
						/*
						if(FLAGDEBUG)	{
							hextemp = tohex(bloom_bP_checksums[i].data,32);
							printf("Checksum %s\n",hextemp);
							free(hextemp);
							bloom_print(&bloom_bP[i]);
						}
						*/
					}
					printf(" Done!\n");
					fclose(fd_aux1);
				}
				else	{
					fprintf(stderr,"[E] Error can't create the file %s\n",buffer_bloom_file);
					exit(0);
				}
			}
			if(!FLAGREADEDFILE2  )	{
				
				snprintf(buffer_bloom_file,1024,"keyhunt_bsgs_6_%" PRIu64 ".blm",bsgs_m2);
								
				/* Writing file for 2nd bloom filter */
				fd_aux2 = fopen(buffer_bloom_file,"wb");
				if(fd_aux2 != NULL)	{
					printf("[+] Writing bloom filter to file %s ",buffer_bloom_file);
					fflush(stdout);
					for(i = 0; i < 256;i++)	{
						readed = fwrite(&bloom_bPx2nd[i],sizeof(struct bloom),1,fd_aux2);
						if(readed != 1)	{
							fprintf(stderr,"[E] Error writing the file %s\n",buffer_bloom_file);
							exit(0);
						}
						readed = fwrite(bloom_bPx2nd[i].bf,bloom_bPx2nd[i].bytes,1,fd_aux2);
						if(readed != 1)	{
							fprintf(stderr,"[E] Error writing the file %s\n",buffer_bloom_file);
							exit(0);
						}
						readed = fwrite(&bloom_bPx2nd_checksums[i],sizeof(struct checksumsha256),1,fd_aux2);
						if(readed != 1)	{
							fprintf(stderr,"[E] Error writing the file %s please delete it\n",buffer_bloom_file);
							exit(0);
						}
						if(i % 64 == 0)	{
							printf(".");
							fflush(stdout);
						}
						/*
						if(FLAGDEBUG)	{
							hextemp = tohex(bloom_bPx2nd_checksum.data,32);
							printf("Checksum %s\n",hextemp);
							free(hextemp);
							bloom_print(&bloom_bPx2nd);
						}
						*/
					}
					printf(" Done!\n");
					fclose(fd_aux2);	
				}
				else	{
					fprintf(stderr,"[E] Error can't create the file %s\n",buffer_bloom_file);
					exit(0);
				}
			}
			
			if(!FLAGREADEDFILE3)	{
				/* Writing file for bPtable */
				snprintf(buffer_bloom_file,1024,"keyhunt_bsgs_2_%" PRIu64 ".tbl",bsgs_m3);
				fd_aux3 = fopen(buffer_bloom_file,"wb");
				if(fd_aux3 != NULL)	{
					printf("[+] Writing bP Table to file %s .. ",buffer_bloom_file);
					fflush(stdout);
					readed = fwrite(bPtable,bytes,1,fd_aux3);
					if(readed != 1)	{
						fprintf(stderr,"[E] Error writing the file %s\n",buffer_bloom_file);
						exit(0);
					}
					readed = fwrite(checksum,32,1,fd_aux3);
					if(readed != 1)	{
						fprintf(stderr,"[E] Error writing the file %s\n",buffer_bloom_file);
						exit(0);
					}
					printf("Done!\n");
					fclose(fd_aux3);	
					/*
					if(FLAGDEBUG)	{
						hextemp = tohex(checksum,32);
						printf("Checksum %s\n",hextemp);
						free(hextemp);
					}
					*/
				}
				else	{
					fprintf(stderr,"[E] Error can't create the file %s\n",buffer_bloom_file);
					exit(0);
				}
			}
			if(!FLAGREADEDFILE4)	{
				snprintf(buffer_bloom_file,1024,"keyhunt_bsgs_7_%" PRIu64 ".blm",bsgs_m3);
								
				/* Writing file for 3rd bloom filter */
				fd_aux2 = fopen(buffer_bloom_file,"wb");
				if(fd_aux2 != NULL)	{
					printf("[+] Writing bloom filter to file %s ",buffer_bloom_file);
					fflush(stdout);
					for(i = 0; i < 256;i++)	{
						readed = fwrite(&bloom_bPx3rd[i],sizeof(struct bloom),1,fd_aux2);
						if(readed != 1)	{
							fprintf(stderr,"[E] Error writing the file %s\n",buffer_bloom_file);
							exit(0);
						}
						readed = fwrite(bloom_bPx3rd[i].bf,bloom_bPx3rd[i].bytes,1,fd_aux2);
						if(readed != 1)	{
							fprintf(stderr,"[E] Error writing the file %s\n",buffer_bloom_file);
							exit(0);
						}
						readed = fwrite(&bloom_bPx3rd_checksums[i],sizeof(struct checksumsha256),1,fd_aux2);
						if(readed != 1)	{
							fprintf(stderr,"[E] Error writing the file %s please delete it\n",buffer_bloom_file);
							exit(0);
						}
						if(i % 64 == 0)	{
							printf(".");
							fflush(stdout);
						}
						/*
						if(FLAGDEBUG)	{
							hextemp = tohex(bloom_bPx2nd_checksum.data,32);
							printf("Checksum %s\n",hextemp);
							free(hextemp);
							bloom_print(&bloom_bPx2nd);
						}
						*/
					}
					printf(" Done!\n");
					fclose(fd_aux2);
				}
				else	{
					fprintf(stderr,"[E] Error can't create the file %s\n",buffer_bloom_file);
					exit(0);
				}
			}
		}


		i = 0;

		steps = (uint64_t *) calloc(NTHREADS,sizeof(uint64_t));
		ends = (unsigned int *) calloc(NTHREADS,sizeof(int));
#if defined(_WIN64) && !defined(__CYGWIN__)
		tid = (HANDLE*)calloc(NTHREADS, sizeof(HANDLE));
#else
		tid = (pthread_t *) calloc(NTHREADS,sizeof(pthread_t));
#endif
		
		for(i= 0;i < NTHREADS; i++)	{
			tt = (tothread*) malloc(sizeof(struct tothread));
			tt->nt = i;
			switch(FLAGBSGSMODE)	{
#if defined(_WIN64) && !defined(__CYGWIN__)
				case 0:
					tid[i] = CreateThread(NULL, 0, thread_process_bsgs, (void*)tt, 0, &s);
					break;
				case 1:
					tid[i] = CreateThread(NULL, 0, thread_process_bsgs_backward, (void*)tt, 0, &s);
					break;
				case 2:
					tid[i] = CreateThread(NULL, 0, thread_process_bsgs_both, (void*)tt, 0, &s);
					break;
				case 3:
					tid[i] = CreateThread(NULL, 0, thread_process_bsgs_random, (void*)tt, 0, &s);
					break;
				case 4:
					tid[i] = CreateThread(NULL, 0, thread_process_bsgs_dance, (void*)tt, 0, &s);
					break;
				}
#else

				case 0:
					s = pthread_create(&tid[i],NULL,thread_process_bsgs,(void *)tt);
				break;
				case 1:
					s = pthread_create(&tid[i],NULL,thread_process_bsgs_backward,(void *)tt);
				break;
				case 2:
					s = pthread_create(&tid[i],NULL,thread_process_bsgs_both,(void *)tt);
				break;
				case 3:
					s = pthread_create(&tid[i],NULL,thread_process_bsgs_random,(void *)tt);
				break;
				case 4:
					s = pthread_create(&tid[i],NULL,thread_process_bsgs_dance,(void *)tt);
				break;
#endif
			}
#if defined(_WIN64) && !defined(__CYGWIN__)
			if (tid[i] == NULL) {
#else
			if(s != 0)	{
#endif
				fprintf(stderr,"[E] thread thread_process\n");
				exit(0);
			}
		}

		
		free(aux);
	}
	if(FLAGMODE != MODE_BSGS)	{
		steps = (uint64_t *) calloc(NTHREADS,sizeof(uint64_t));
		ends = (unsigned int *) calloc(NTHREADS,sizeof(int));
#if defined(_WIN64) && !defined(__CYGWIN__)
		tid = (HANDLE*)calloc(NTHREADS, sizeof(HANDLE));
#else
		tid = (pthread_t *) calloc(NTHREADS,sizeof(pthread_t));
#endif
		for(i= 0;i < NTHREADS; i++)	{
			tt = (tothread*) malloc(sizeof(struct tothread));
			tt->nt = i;
			steps[i] = 0;
			switch(FLAGMODE)	{
#if defined(_WIN64) && !defined(__CYGWIN__)
				case MODE_ADDRESS:
				case MODE_XPOINT:
				case MODE_RMD160:
					tid[i] = CreateThread(NULL, 0, thread_process, (void*)tt, 0, &s);
				break;
				case MODE_PUB2RMD:
					tid[i] = CreateThread(NULL, 0, thread_pub2rmd, (void*)tt, 0, &s);
				break;
				case MODE_MINIKEYS:
					tid[i] = CreateThread(NULL, 0, thread_process_minikeys, (void*)tt, 0, &s);
				break;
#else
				case MODE_ADDRESS:
				case MODE_XPOINT:
				case MODE_RMD160:
					s = pthread_create(&tid[i],NULL,thread_process,(void *)tt);
				break;
				case MODE_PUB2RMD:
					s = pthread_create(&tid[i],NULL,thread_pub2rmd,(void *)tt);
				break;
				case MODE_MINIKEYS:
					s = pthread_create(&tid[i],NULL,thread_process_minikeys,(void *)tt);
				break;
				/*
				case MODE_CHECK:
					s = pthread_create(&tid[i],NULL,thread_process_check_file_btc,(void *)tt);
				break;
				*/
#endif
			}
			if(s != 0)	{
				fprintf(stderr,"[E] pthread_create thread_process\n");
				exit(0);
			}
		}
	}
	i = 0;
	
	while(i < 7)	{
		int_limits[i].SetBase10((char*)str_limits[i]);
		i++;
	}
	
	continue_flag = 1;
	total.SetInt32(0);
	pretotal.SetInt32(0);
	debugcount_mpz.Set(&BSGS_N);
	seconds.SetInt32(0);
	do	{
		sleep_ms(1000);
		seconds.AddOne();
		check_flag = 1;
		for(i = 0; i <NTHREADS && check_flag; i++) {
			check_flag &= ends[i];
		}
		if(check_flag)	{
			continue_flag = 0;
		}
		if(OUTPUTSECONDS.IsGreater(&ZERO) ){
			MPZAUX.Set(&seconds);
			MPZAUX.Mod(&OUTPUTSECONDS);
			if(MPZAUX.IsZero()) {
				total.SetInt32(0);
				i = 0;
				while(i < NTHREADS) {
					pretotal.Set(&debugcount_mpz);
					pretotal.Mult(steps[i]);
					total.Add(&pretotal);
					i++;
				}
#ifdef _WIN64
				WaitForSingleObject(bsgs_thread, INFINITE);
#else
				pthread_mutex_lock(&bsgs_thread);
#endif			
				pretotal.Set(&total);
				pretotal.Div(&seconds);
				str_seconds = seconds.GetBase10();
				str_pretotal = pretotal.GetBase10();
				str_total = total.GetBase10();
				
				
				if(pretotal.IsLower(&int_limits[0]))	{
					if(FLAGMATRIX)	{
						sprintf(buffer,"[+] Total %s keys in %s seconds: %s keys/s\n",str_total,str_seconds,str_pretotal);
					}
					else	{
						sprintf(buffer,"\r[+] Total %s keys in %s seconds: %s keys/s\r",str_total,str_seconds,str_pretotal);
					}
				}
				else	{
					i = 0;
					salir = 0;
					while( i < 6 && !salir)	{
						if(pretotal.IsLower(&int_limits[i+1]))	{
							salir = 1;
						}
						else	{
							i++;
						}
					}

					div_pretotal.Set(&pretotal);
					div_pretotal.Div(&int_limits[salir ? i : i-1]);
					str_divpretotal = div_pretotal.GetBase10();
					if(FLAGMATRIX)	{
						sprintf(buffer,"[+] Total %s keys in %s seconds: ~%s %s (%s keys/s)\n",str_total,str_seconds,str_divpretotal,str_limits_prefixs[salir ? i : i-1],str_pretotal);
					}
					else	{
						if(THREADOUTPUT == 1)	{
							sprintf(buffer,"\r[+] Total %s keys in %s seconds: ~%s %s (%s keys/s)\r",str_total,str_seconds,str_divpretotal,str_limits_prefixs[salir ? i : i-1],str_pretotal);
						}
						else	{
							sprintf(buffer,"\r[+] Total %s keys in %s seconds: ~%s %s (%s keys/s)\r",str_total,str_seconds,str_divpretotal,str_limits_prefixs[salir ? i : i-1],str_pretotal);
						}
					}
					free(str_divpretotal);

				}
				printf("%s",buffer);
				fflush(stdout);
				THREADOUTPUT = 0;			
#ifdef _WIN64
				ReleaseMutex(bsgs_thread);
#else
				pthread_mutex_unlock(&bsgs_thread);
#endif

				free(str_seconds);
				free(str_pretotal);
				free(str_total);
			}
		}
	}while(continue_flag);
	printf("\nEnd\n");
#ifdef _WIN64
	CloseHandle(write_keys);
	CloseHandle(write_random);
	CloseHandle(bsgs_thread);
#endif
}

void pubkeytopubaddress_dst(char *pkey,int length,char *dst)	{
	char digest[60];
	size_t pubaddress_size = 40;
	sha256((uint8_t*)pkey, length,(uint8_t*) digest);
	RMD160Data((const unsigned char*)digest,32, digest+1);
	digest[0] = 0;
	sha256((uint8_t*)digest, 21,(uint8_t*) digest+21);
	sha256((uint8_t*)digest+21, 32,(uint8_t*) digest+21);
	if(!b58enc(dst,&pubaddress_size,digest,25)){
		fprintf(stderr,"error b58enc\n");
	}
}

void rmd160toaddress_dst(char *rmd,char *dst){
	char digest[60];
	size_t pubaddress_size = 40;
	digest[0] = byte_encode_crypto;
	memcpy(digest+1,rmd,20);
	sha256((uint8_t*)digest, 21,(uint8_t*) digest+21);
	sha256((uint8_t*)digest+21, 32,(uint8_t*) digest+21);
	if(!b58enc(dst,&pubaddress_size,digest,25)){
		fprintf(stderr,"error b58enc\n");
	}
}


char *pubkeytopubaddress(char *pkey,int length)	{
	char *pubaddress = (char*) calloc(MAXLENGTHADDRESS+10,1);
	char *digest = (char*) calloc(60,1);
	size_t pubaddress_size = MAXLENGTHADDRESS+10;
	if(pubaddress == NULL || digest == NULL)	{
		fprintf(stderr,"error malloc()\n");
		exit(0);
	}
	//digest [000...0]
 	sha256((uint8_t*)pkey, length,(uint8_t*) digest);
	//digest [SHA256 32 bytes+000....0]
	RMD160Data((const unsigned char*)digest,32, digest+1);
	//digest [? +RMD160 20 bytes+????000....0]
	digest[0] = 0;
	//digest [0 +RMD160 20 bytes+????000....0]
	sha256((uint8_t*)digest, 21,(uint8_t*) digest+21);
	//digest [0 +RMD160 20 bytes+SHA256 32 bytes+....0]
	sha256((uint8_t*)digest+21, 32,(uint8_t*) digest+21);
	//digest [0 +RMD160 20 bytes+SHA256 32 bytes+....0]
	if(!b58enc(pubaddress,&pubaddress_size,digest,25)){
		fprintf(stderr,"error b58enc\n");
	}
	free(digest);
	return pubaddress;	// pubaddress need to be free by te caller funtion
}

void publickeytohashrmd160_dst(char *pkey,int length,char *dst)	{
	char digest[32];
	//digest [000...0]
 	sha256((uint8_t*)pkey, length,(uint8_t*) digest);
	//digest [SHA256 32 bytes]
	RMD160Data((const unsigned char*)digest,32, dst);
	//hash160 [RMD160 20 bytes]
}

char *publickeytohashrmd160(char *pkey,int length)	{
	char *hash160 = (char*) malloc(20);
	char *digest = (char*) malloc(32);
	if(hash160 == NULL || digest == NULL)	{
		fprintf(stderr,"error malloc()\n");
		exit(0);
	}
	//digest [000...0]
 	sha256((uint8_t*)pkey, length,(uint8_t*) digest);
	//digest [SHA256 32 bytes]
	RMD160Data((const unsigned char*)digest,32, hash160);
	//hash160 [RMD160 20 bytes]
	free(digest);
	return hash160;	// hash160 need to be free by te caller funtion
}

int searchbinary(struct address_value *buffer,char *data,int64_t array_length) {
	int64_t half,min,max,current;
	int r = 0,rcmp;
	min = 0;
	current = 0;
	max = array_length;
	half = array_length;
	while(!r && half >= 1) {
		half = (max - min)/2;
		rcmp = memcmp(data,buffer[current+half].value,20);
		if(rcmp == 0)	{
			r = 1;	//Found!!
		}
		else	{
			if(rcmp < 0) { //data < temp_read
				max = (max-half);
			}
			else	{ // data > temp_read
				min = (min+half);
			}
			current = min;
		}
	}
	return r;
}

#if defined(_WIN64) && !defined(__CYGWIN__)
DWORD WINAPI thread_process_minikeys(LPVOID vargp) {
#else
void *thread_process_minikeys(void *vargp)	{
#endif
	FILE *keys;
	Point publickey[4];
	Int key_mpz[4];
	struct tothread *tt;
	uint64_t count;
	char publickeyhashrmd160_uncompress[4][20];
	char public_key_compressed_hex[67],public_key_uncompressed_hex[131];
	char hexstrpoint[65],rawvalue[4][32];
	char address[4][40],minikey[4][24],minikeys[8][24], buffer[1024],digest[32],buffer_b58[21],minikey2check[24];
	char *hextemp,*rawbuffer;
	int r,thread_number,continue_flag = 1,k,j,count_valid;
	Int counter;
	tt = (struct tothread *)vargp;
	thread_number = tt->nt;
	free(tt);
	rawbuffer = (char*) &counter.bits64;
	count_valid = 0;
	for(k = 0; k < 4; k++)	{
		minikey[k][0] = 'S';
		minikey[k][22] = '?';
		minikey[k][23] = 0x00;
	}
	minikey2check[0] = 'S';
	minikey2check[22] = '?';
	minikey2check[23] = 0x00;
	
	do	{
		if(FLAGRANDOM)	{
#if defined(_WIN64) && !defined(__CYGWIN__)
			WaitForSingleObject(write_random, INFINITE);
			counter.Rand(256);
			ReleaseMutex(write_random);
#else
			pthread_mutex_lock(&write_random);
			counter.Rand(256);
			pthread_mutex_unlock(&write_random);
#endif
			for(k = 0; k < 21; k++)	{
				buffer_b58[k] =(uint8_t)((uint8_t) rawbuffer[k] % 58);
			}
		}
		else	{
			if(FLAGBASEMINIKEY)	{
#if defined(_WIN64) && !defined(__CYGWIN__)
				WaitForSingleObject(write_random, INFINITE);
				memcpy(buffer_b58,raw_baseminikey,21);
				increment_minikey_N(raw_baseminikey);
				ReleaseMutex(write_random);
#else
				pthread_mutex_lock(&write_random);
				memcpy(buffer_b58,raw_baseminikey,21);
				increment_minikey_N(raw_baseminikey);
				pthread_mutex_unlock(&write_random);
#endif
			}
			else	{
#if defined(_WIN64) && !defined(__CYGWIN__)
				WaitForSingleObject(write_random, INFINITE);
#else
				pthread_mutex_lock(&write_random);
#endif
				if(raw_baseminikey == NULL){
					raw_baseminikey = (char *) malloc(22);
					if( raw_baseminikey == NULL)	{
						fprintf(stderr,"[E] malloc()\n");
						exit(0);
					}
					counter.Rand(256);
					for(k = 0; k < 21; k++)	{
						raw_baseminikey[k] =(uint8_t)((uint8_t) rawbuffer[k] % 58);
					}
					memcpy(buffer_b58,raw_baseminikey,21);
					increment_minikey_N(raw_baseminikey);

				}
				else	{
					memcpy(buffer_b58,raw_baseminikey,21);
					increment_minikey_N(raw_baseminikey);
				}
#if defined(_WIN64) && !defined(__CYGWIN__)				
				ReleaseMutex(write_random);
#else
				pthread_mutex_unlock(&write_random);
#endif
				
			}
		}
		set_minikey(minikey2check+1,buffer_b58,21);
		if(continue_flag)	{
			count = 0;
			if(FLAGMATRIX)	{
					printf("[+] Base minikey: %s     \n",minikey2check);
					fflush(stdout);
			}
			else	{
				if(!FLAGQUIET)	{
					printf("\r[+] Base minikey: %s     \r",minikey2check);
					fflush(stdout);
				}
			}
			do {
				for(j = 0;j<256; j++)	{
					
					if(count_valid > 0)	{
						for(k = 0; k < count_valid ; k++)	{
							memcpy(minikeys[k],minikeys[4+k],22);
						}
					}
					do	{
						increment_minikey_index(minikey2check+1,buffer_b58,20);
						memcpy(minikey[0]+1,minikey2check+1,21);
						increment_minikey_index(minikey2check+1,buffer_b58,20);
						memcpy(minikey[1]+1,minikey2check+1,21);
						increment_minikey_index(minikey2check+1,buffer_b58,20);
						memcpy(minikey[2]+1,minikey2check+1,21);
						increment_minikey_index(minikey2check+1,buffer_b58,20);
						memcpy(minikey[3]+1,minikey2check+1,21);
						
						sha256sse_23((uint8_t*)minikey[0],(uint8_t*)minikey[1],(uint8_t*)minikey[2],(uint8_t*)minikey[3],(uint8_t*)rawvalue[0],(uint8_t*)rawvalue[1],(uint8_t*)rawvalue[2],(uint8_t*)rawvalue[3]);
						for(k = 0; k < 4; k++){
							if(rawvalue[k][0] == 0x00)	{
								memcpy(minikeys[count_valid],minikey[k],22);
								count_valid++;
							}
						}
					}while(count_valid < 4);
					count_valid-=4;				
					sha256sse_22((uint8_t*)minikeys[0],(uint8_t*)minikeys[1],(uint8_t*)minikeys[2],(uint8_t*)minikeys[3],(uint8_t*)rawvalue[0],(uint8_t*)rawvalue[1],(uint8_t*)rawvalue[2],(uint8_t*)rawvalue[3]);
					
					for(k = 0; k < 4; k++)	{
						key_mpz[k].Set32Bytes((uint8_t*)rawvalue[k]);
						publickey[k] = secp->ComputePublicKey(&key_mpz[k]);
					}
					
					secp->GetHash160(P2PKH,false,publickey[0],publickey[1],publickey[2],publickey[3],(uint8_t*)publickeyhashrmd160_uncompress[0],(uint8_t*)publickeyhashrmd160_uncompress[1],(uint8_t*)publickeyhashrmd160_uncompress[2],(uint8_t*)publickeyhashrmd160_uncompress[3]);
					
					for(k = 0; k < 4; k++)	{
						r = bloom_check(&bloom,publickeyhashrmd160_uncompress[k],20);
						if(r) {
							r = searchbinary(addressTable,publickeyhashrmd160_uncompress[k],N);
							if(r) {
								/* hit */
								hextemp = key_mpz[k].GetBase16();
								secp->GetPublicKeyHex(false,publickey[k],public_key_uncompressed_hex);
#if defined(_WIN64) && !defined(__CYGWIN__)
								WaitForSingleObject(write_keys, INFINITE);
#else
								pthread_mutex_lock(&write_keys);
#endif
							
								keys = fopen("KEYFOUNDKEYFOUND.txt","a+");
								rmd160toaddress_dst(publickeyhashrmd160_uncompress[k],address[k]);
								minikeys[k][22] = '\0';
								if(keys != NULL)	{
									fprintf(keys,"Private Key: %s\npubkey: %s\nminikey: %s\naddress: %s\n",hextemp,public_key_uncompressed_hex,minikeys[k],address[k]);
									fclose(keys);
								}
								printf("\nHIT!! Private Key: %s\npubkey: %s\nminikey: %s\naddress: %s\n",hextemp,public_key_uncompressed_hex,minikeys[k],address[k]);
#if defined(_WIN64) && !defined(__CYGWIN__)
								ReleaseMutex(write_keys);
#else
								pthread_mutex_unlock(&write_keys);
#endif
								
								free(hextemp);
							}
						}
					}
				}
				steps[thread_number]++;
				count+=1024;
			}while(count < N_SECUENTIAL_MAX && continue_flag);
		}
	}while(continue_flag);
	return NULL;
}



#if defined(_WIN64) && !defined(__CYGWIN__)
DWORD WINAPI thread_process(LPVOID vargp) {
#else
void *thread_process(void *vargp)	{
#endif
	struct tothread *tt;
	Point pts[CPU_GRP_SIZE];
	Int dx[CPU_GRP_SIZE / 2 + 1];
	IntGroup *grp = new IntGroup(CPU_GRP_SIZE / 2 + 1);
	Point startP;
	Int dy;
	Int dyn;
	Int _s;
	Int _p;
	Point pp;
	Point pn;
	int hLength = (CPU_GRP_SIZE / 2 - 1);
	uint64_t i,j,count;
	Point R,temporal;
	int r,thread_number,continue_flag = 1,i_vanity,k;
	char *hextemp = NULL;
	char *eth_address = NULL;
	char publickeyhashrmd160_uncompress[4][20],publickeyhashrmd160_compress[4][20];
	char public_key_compressed_hex[67],public_key_uncompressed_hex[131];
	char public_key_compressed[33],public_key_uncompressed[65];
	char hexstrpoint[65],rawvalue[32];
	char address_compressed[4][40],address_uncompressed[4][40];
	
	unsigned long longtemp;
	FILE *keys,*vanityKeys;
	Int key_mpz,keyfound,temp_stride;
	tt = (struct tothread *)vargp;
	thread_number = tt->nt;
	free(tt);
	grp->Set(dx);
	

	do {
		if(FLAGRANDOM){
			key_mpz.Rand(&n_range_start,&n_range_end);
		}
		else	{
			if(n_range_start.IsLower(&n_range_end))	{
#if defined(_WIN64) && !defined(__CYGWIN__)
				WaitForSingleObject(write_random, INFINITE);
				key_mpz.Set(&n_range_start);
				n_range_start.Add(N_SECUENTIAL_MAX);
				ReleaseMutex(write_random);
#else
				pthread_mutex_lock(&write_random);
				key_mpz.Set(&n_range_start);
				n_range_start.Add(N_SECUENTIAL_MAX);
				pthread_mutex_unlock(&write_random);
#endif
			}
			else	{
				continue_flag = 0;
			}
		}
		if(continue_flag)	{
			count = 0;
			if(FLAGMATRIX)	{
					hextemp = key_mpz.GetBase16();
					printf("Base key: %s thread %i\n",hextemp,thread_number);
					fflush(stdout);
					free(hextemp);
			}
			else	{
				if(FLAGQUIET == 0){
					hextemp = key_mpz.GetBase16();
					printf("\rBase key: %s     \r",hextemp);
					fflush(stdout);
					free(hextemp);
					THREADOUTPUT = 1;
				}
			}
			do {
				temp_stride.SetInt32(CPU_GRP_SIZE / 2);
				temp_stride.Mult(&stride);
				key_mpz.Add(&temp_stride);
	 			startP = secp->ComputePublicKey(&key_mpz);
				key_mpz.Sub(&temp_stride);

				for(i = 0; i < hLength; i++) {
					dx[i].ModSub(&Gn[i].x,&startP.x);
				}
			
				dx[i].ModSub(&Gn[i].x,&startP.x);  // For the first point
				dx[i + 1].ModSub(&_2Gn.x,&startP.x); // For the next center point
				grp->ModInv();

				pts[CPU_GRP_SIZE / 2] = startP;

				for(i = 0; i<hLength; i++) {
					pp = startP;
					pn = startP;

					// P = startP + i*G
					dy.ModSub(&Gn[i].y,&pp.y);

					_s.ModMulK1(&dy,&dx[i]);        // s = (p2.y-p1.y)*inverse(p2.x-p1.x);
					_p.ModSquareK1(&_s);            // _p = pow2(s)

					pp.x.ModNeg();
					pp.x.ModAdd(&_p);
					pp.x.ModSub(&Gn[i].x);           // rx = pow2(s) - p1.x - p2.x;

					if(FLAGMODE != MODE_XPOINT  )	{
						pp.y.ModSub(&Gn[i].x,&pp.x);
						pp.y.ModMulK1(&_s);
						pp.y.ModSub(&Gn[i].y);           // ry = - p2.y - s*(ret.x-p2.x);
					}

					// P = startP - i*G  , if (x,y) = i*G then (x,-y) = -i*G
					dyn.Set(&Gn[i].y);
					dyn.ModNeg();
					dyn.ModSub(&pn.y);

					_s.ModMulK1(&dyn,&dx[i]);      // s = (p2.y-p1.y)*inverse(p2.x-p1.x);
					_p.ModSquareK1(&_s);            // _p = pow2(s)
					pn.x.ModNeg();
					pn.x.ModAdd(&_p);
					pn.x.ModSub(&Gn[i].x);          // rx = pow2(s) - p1.x - p2.x;

					if(FLAGMODE != MODE_XPOINT  )	{
						pn.y.ModSub(&Gn[i].x,&pn.x);
						pn.y.ModMulK1(&_s);
						pn.y.ModAdd(&Gn[i].y);          // ry = - p2.y - s*(ret.x-p2.x);
					}

					pts[CPU_GRP_SIZE / 2 + (i + 1)] = pp;
					pts[CPU_GRP_SIZE / 2 - (i + 1)] = pn;
				}

				// First point (startP - (GRP_SZIE/2)*G)
				pn = startP;
				dyn.Set(&Gn[i].y);
				dyn.ModNeg();
				dyn.ModSub(&pn.y);

				_s.ModMulK1(&dyn,&dx[i]);
				_p.ModSquareK1(&_s);

				pn.x.ModNeg();
				pn.x.ModAdd(&_p);
				pn.x.ModSub(&Gn[i].x);
				
				if(FLAGMODE != MODE_XPOINT  )	{
					pn.y.ModSub(&Gn[i].x,&pn.x);
					pn.y.ModMulK1(&_s);
					pn.y.ModAdd(&Gn[i].y);
				}

				pts[0] = pn;
				for(j = 0; j < CPU_GRP_SIZE/4;j++){
					if(FLAGCRYPTO == CRYPTO_BTC){
						switch(FLAGMODE)	{
							case MODE_ADDRESS:
							case MODE_RMD160:
								switch(FLAGSEARCH)	{
									case SEARCH_UNCOMPRESS:
										secp->GetHash160(P2PKH,false,pts[(j*4)],pts[(j*4)+1],pts[(j*4)+2],pts[(j*4)+3],(uint8_t*)publickeyhashrmd160_uncompress[0],(uint8_t*)publickeyhashrmd160_uncompress[1],(uint8_t*)publickeyhashrmd160_uncompress[2],(uint8_t*)publickeyhashrmd160_uncompress[3]);
									break;
									case SEARCH_COMPRESS:
										secp->GetHash160(P2PKH,true,pts[(j*4)],pts[(j*4)+1],pts[(j*4)+2],pts[(j*4)+3],(uint8_t*)publickeyhashrmd160_compress[0],(uint8_t*)publickeyhashrmd160_compress[1],(uint8_t*)publickeyhashrmd160_compress[2],(uint8_t*)publickeyhashrmd160_compress[3]);
									break;
									case SEARCH_BOTH:
										secp->GetHash160(P2PKH,true,pts[(j*4)],pts[(j*4)+1],pts[(j*4)+2],pts[(j*4)+3],(uint8_t*)publickeyhashrmd160_compress[0],(uint8_t*)publickeyhashrmd160_compress[1],(uint8_t*)publickeyhashrmd160_compress[2],(uint8_t*)publickeyhashrmd160_compress[3]);
										secp->GetHash160(P2PKH,false,pts[(j*4)],pts[(j*4)+1],pts[(j*4)+2],pts[(j*4)+3],(uint8_t*)publickeyhashrmd160_uncompress[0],(uint8_t*)publickeyhashrmd160_uncompress[1],(uint8_t*)publickeyhashrmd160_uncompress[2],(uint8_t*)publickeyhashrmd160_uncompress[3]);
									break;
								}
							break;
						}
					}
					switch(FLAGMODE)	{
						case MODE_ADDRESS:
							if( FLAGCRYPTO  == CRYPTO_BTC) {
								switch(FLAGSEARCH)	{
									case SEARCH_UNCOMPRESS:
										for(k = 0; k < 4;k++)	{
											rmd160toaddress_dst(publickeyhashrmd160_uncompress[k],address_uncompressed[k]);
										}
									break;
									case SEARCH_COMPRESS:
										for(k = 0; k < 4;k++)	{
											rmd160toaddress_dst(publickeyhashrmd160_compress[k],address_compressed[k]);
										}
									break;
									case SEARCH_BOTH:
										for(k = 0; k < 4;k++)	{
											rmd160toaddress_dst(publickeyhashrmd160_uncompress[k],address_uncompressed[k]);
											rmd160toaddress_dst(publickeyhashrmd160_compress[k],address_compressed[k]);
										}
									break;
								}
								if(FLAGVANITY)	{
									for(k = 0; k < 4;k++)	{
										i_vanity = 0;
										while(i_vanity < COUNT_VANITIES)	{
											if(FLAGSEARCH == SEARCH_UNCOMPRESS || FLAGSEARCH == SEARCH_BOTH){
												if(strncmp(address_uncompressed[k],vanities[i_vanity],len_vanities[i_vanity]) == 0)	{
													keyfound.SetInt32(k);
													keyfound.Mult(&stride);
													keyfound.Add(&key_mpz);
													hextemp = keyfound.GetBase16();
													vanityKeys = fopen("vanitykeys.txt","a+");
													if(vanityKeys != NULL)	{
														fprintf(vanityKeys,"Private Key: %s\nAddress uncompressed: %s\n",hextemp,address_uncompressed[k]);
														fclose(vanityKeys);
													}
													printf("\nVanity Private Key: %s\nAddress uncompressed:	%s\n",hextemp,address_uncompressed[k]);
													free(hextemp);
												}
											}
											if(FLAGSEARCH == SEARCH_COMPRESS || FLAGSEARCH == SEARCH_BOTH){
												if(strncmp(address_compressed[k],vanities[i_vanity],len_vanities[i_vanity]) == 0)	{
													keyfound.SetInt32(k);
													keyfound.Mult(&stride);
													keyfound.Add(&key_mpz);
													hextemp = keyfound.GetBase16();
													vanityKeys = fopen("vanitykeys.txt","a+");
													if(vanityKeys != NULL)	{
														fprintf(vanityKeys,"Private key: %s\nAddress compressed:	%s\n",hextemp,address_compressed[k]);
														fclose(vanityKeys);
													}
													printf("\nVanity Private Key: %s\nAddress compressed: %s\n",hextemp,address_compressed[k]);
													free(hextemp);
												}
											}
											i_vanity ++;
										}
									}
								}
								if(FLAGSEARCH == SEARCH_COMPRESS || FLAGSEARCH == SEARCH_BOTH){
									for(k = 0; k < 4;k++)	{
										r = bloom_check(&bloom,address_compressed[k],MAXLENGTHADDRESS);
										if(r) {
											r = searchbinary(addressTable,address_compressed[k],N);
											if(r) {
												keyfound.SetInt32(k);
												keyfound.Mult(&stride);
												keyfound.Add(&key_mpz);
												hextemp = keyfound.GetBase16();
												secp->GetPublicKeyHex(true,pts[(4*j)+k],public_key_compressed_hex);

#if defined(_WIN64) && !defined(__CYGWIN__)
												WaitForSingleObject(write_keys, INFINITE);
#else
												pthread_mutex_lock(&write_keys);
#endif

												keys = fopen("KEYFOUNDKEYFOUND.txt","a+");
												if(keys != NULL)	{
													fprintf(keys,"Private Key: %s\npubkey: %s\naddress: %s\n",hextemp,public_key_compressed_hex,address_compressed[k]);
													fclose(keys);
												}
												printf("\nHIT!! Private Key: %s\npubkey: %s\naddress: %s\n",hextemp,public_key_compressed_hex,address_compressed[k]);
#if defined(_WIN64) && !defined(__CYGWIN__)
												ReleaseMutex(write_keys);
#else
												pthread_mutex_unlock(&write_keys);
#endif

												free(hextemp);
											}
										}
									}
								}

								if(FLAGSEARCH == SEARCH_UNCOMPRESS || FLAGSEARCH == SEARCH_BOTH){
									for(k = 0; k < 4;k++)	{
										r = bloom_check(&bloom,address_uncompressed[k],MAXLENGTHADDRESS);
										if(r) {
											r = searchbinary(addressTable,address_uncompressed[k],N);
											if(r) {
												keyfound.SetInt32(k);
												keyfound.Mult(&stride);
												keyfound.Add(&key_mpz);
												hextemp = keyfound.GetBase16();
												secp->GetPublicKeyHex(false,pts[(4*j)+k],public_key_uncompressed_hex);
#if defined(_WIN64) && !defined(__CYGWIN__)
												WaitForSingleObject(write_keys, INFINITE);
#else
												pthread_mutex_lock(&write_keys);
#endif

												keys = fopen("KEYFOUNDKEYFOUND.txt","a+");
												if(keys != NULL)	{
													fprintf(keys,"Private Key: %s\npubkey: %s\naddress: %s\n",hextemp,public_key_uncompressed_hex,address_uncompressed[k]);
													fclose(keys);
												}
												printf("\nHIT!! Private Key: %s\npubkey: %s\naddress: %s\n",hextemp,public_key_uncompressed_hex,address_uncompressed[k]);
#if defined(_WIN64) && !defined(__CYGWIN__)
												ReleaseMutex(write_keys);
#else
												pthread_mutex_unlock(&write_keys);
#endif
												free(hextemp);
											}
										}
									}

								}
							}
							if( FLAGCRYPTO == CRYPTO_ETH) {
								for(k = 0; k < 4;k++)	{
									generate_binaddress_eth(pts[(4*j)+k],(unsigned char*)rawvalue);
									/*
									hextemp = tohex(rawvalue+12,20);
									printf("Eth Address 0x%s\n",hextemp);
									free(hextemp);
									*/
									r = bloom_check(&bloom,rawvalue+12,MAXLENGTHADDRESS);
									if(r) {
										r = searchbinary(addressTable,rawvalue+12,N);
										if(r) {
											keyfound.SetInt32(k);
											keyfound.Mult(&stride);
											keyfound.Add(&key_mpz);
											hextemp = keyfound.GetBase16();
											hexstrpoint[0] = '0';
											hexstrpoint[1] = 'x';
											tohex_dst(rawvalue+12,20,hexstrpoint+2);

#if defined(_WIN64) && !defined(__CYGWIN__)
											WaitForSingleObject(write_keys, INFINITE);
#else
											pthread_mutex_lock(&write_keys);
#endif

											keys = fopen("KEYFOUNDKEYFOUND.txt","a+");
											if(keys != NULL)	{
												fprintf(keys,"Private Key: %s\naddress: %s\n",hextemp,hexstrpoint);
												fclose(keys);
											}
											printf("\n Hit!!!! Private Key: %s\naddress: %s\n",hextemp,hexstrpoint);
#if defined(_WIN64) && !defined(__CYGWIN__)
											ReleaseMutex(write_keys);
#else
											pthread_mutex_unlock(&write_keys);
#endif

											free(hextemp);
										}
									}
								}

							}
						break;
						case MODE_RMD160:
							for(k = 0; k < 4;k++)	{
								if(FLAGSEARCH == SEARCH_COMPRESS || FLAGSEARCH == SEARCH_BOTH){
									r = bloom_check(&bloom,publickeyhashrmd160_compress[k],MAXLENGTHADDRESS);
									if(r) {
										r = searchbinary(addressTable,publickeyhashrmd160_compress[k],N);
										if(r) {
											keyfound.SetInt32(k);
											keyfound.Mult(&stride);
											keyfound.Add(&key_mpz);
											hextemp = keyfound.GetBase16();
											secp->GetPublicKeyHex(true,pts[(4*j)+k],public_key_compressed_hex);
#if defined(_WIN64) && !defined(__CYGWIN__)
											WaitForSingleObject(write_keys, INFINITE);
#else
											pthread_mutex_lock(&write_keys);
#endif

											keys = fopen("KEYFOUNDKEYFOUND.txt","a+");
											if(keys != NULL)	{
												fprintf(keys,"Private Key: %s\npubkey: %s\n",hextemp,public_key_compressed_hex);
												fclose(keys);
											}
											printf("\nHIT!! Private Key: %s\npubkey: %s\n",hextemp,public_key_compressed_hex);
#if defined(_WIN64) && !defined(__CYGWIN__)
											ReleaseMutex(write_keys);
#else
											pthread_mutex_unlock(&write_keys);
#endif
											free(hextemp);
										}
									}
								}
								if(FLAGSEARCH == SEARCH_UNCOMPRESS || FLAGSEARCH == SEARCH_BOTH)	{
									r = bloom_check(&bloom,publickeyhashrmd160_uncompress[k],MAXLENGTHADDRESS);
									if(r) {
										r = searchbinary(addressTable,publickeyhashrmd160_uncompress[k],N);
										if(r) {
											keyfound.SetInt32(k);
											keyfound.Mult(&stride);
											keyfound.Add(&key_mpz);
											hextemp = keyfound.GetBase16();

											secp->GetPublicKeyHex(false,pts[(4*j)+k],public_key_uncompressed_hex);
#if defined(_WIN64) && !defined(__CYGWIN__)
											WaitForSingleObject(write_keys, INFINITE);
#else
											pthread_mutex_lock(&write_keys);
#endif
											keys = fopen("KEYFOUNDKEYFOUND.txt","a+");
											if(keys != NULL)	{
												fprintf(keys,"Private Key: %s\npubkey: %s\n",hextemp,public_key_uncompressed_hex);
												fclose(keys);
											}
											printf("\nHIT!! Private Key: %s\npubkey: %s\n",hextemp,public_key_uncompressed_hex);
#if defined(_WIN64) && !defined(__CYGWIN__)
											ReleaseMutex(write_keys);
#else
											pthread_mutex_unlock(&write_keys);
#endif
											free(hextemp);
										}
									}
								}
							}
						break;
						case MODE_XPOINT:
							for(k = 0; k < 4;k++)	{
								pts[(4*j)+k].x.Get32Bytes((unsigned char *)rawvalue);
								r = bloom_check(&bloom,rawvalue,MAXLENGTHADDRESS);
								if(r) {
									r = searchbinary(addressTable,rawvalue,N);
									if(r) {
										keyfound.SetInt32(k);
										keyfound.Mult(&stride);
										keyfound.Add(&key_mpz);
										hextemp = keyfound.GetBase16();
										R = secp->ComputePublicKey(&keyfound);
										secp->GetPublicKeyHex(true,R,public_key_compressed_hex);
										printf("\nHIT!! Private Key: %s\npubkey: %s\n",hextemp,public_key_compressed_hex);
#if defined(_WIN64) && !defined(__CYGWIN__)
										WaitForSingleObject(write_keys, INFINITE);
#else
										pthread_mutex_lock(&write_keys);
#endif

										keys = fopen("KEYFOUNDKEYFOUND.txt","a+");
										if(keys != NULL)	{
											fprintf(keys,"Private Key: %s\npubkey: %s\n",hextemp,public_key_compressed_hex);
											fclose(keys);
										}
#if defined(_WIN64) && !defined(__CYGWIN__)
										ReleaseMutex(write_keys);
#else
										pthread_mutex_unlock(&write_keys);
#endif

										free(hextemp);
									}
								}
							}
						break;
					}
					count+=4;
					temp_stride.SetInt32(4);
					temp_stride.Mult(&stride);
					key_mpz.Add(&temp_stride);
				}
				steps[thread_number]++;

				// Next start point (startP + GRP_SIZE*G)
				pp = startP;
				dy.ModSub(&_2Gn.y,&pp.y);

				_s.ModMulK1(&dy,&dx[i + 1]);
				_p.ModSquareK1(&_s);

				pp.x.ModNeg();
				pp.x.ModAdd(&_p);
				pp.x.ModSub(&_2Gn.x);

				pp.y.ModSub(&_2Gn.x,&pp.x);
				pp.y.ModMulK1(&_s);
				pp.y.ModSub(&_2Gn.y);
				startP = pp;
			}while(count < N_SECUENTIAL_MAX && continue_flag);
		}
	} while(continue_flag);
	ends[thread_number] = 1;
	return NULL;
}


void _swap(struct address_value *a,struct address_value *b)	{
	struct address_value t;
	t  = *a;
	*a = *b;
	*b =  t;
}

void _sort(struct address_value *arr,int64_t n)	{
	uint32_t depthLimit = ((uint32_t) ceil(log(n))) * 2;
	_introsort(arr,depthLimit,n);
}

void _introsort(struct address_value *arr,uint32_t depthLimit, int64_t n) {
	int64_t p;
	if(n > 1)	{
		if(n <= 16) {
			_insertionsort(arr,n);
		}
		else	{
			if(depthLimit == 0) {
				_myheapsort(arr,n);
			}
			else	{
				p = _partition(arr,n);
				if(p > 0) _introsort(arr , depthLimit-1 , p);
				if(p < n) _introsort(&arr[p+1],depthLimit-1,n-(p+1));
			}
		}
	}
}

void _insertionsort(struct address_value *arr, int64_t n) {
	int64_t j;
	int64_t i;
	struct address_value key;
	for(i = 1; i < n ; i++ ) {
		key = arr[i];
		j= i-1;
		while(j >= 0 && memcmp(arr[j].value,key.value,20) > 0) {
			arr[j+1] = arr[j];
			j--;
		}
		arr[j+1] = key;
	}
}

int64_t _partition(struct address_value *arr, int64_t n)	{
	struct address_value pivot;
	int64_t r,left,right;
	char *hextemp;
	r = n/2;
	pivot = arr[r];
	left = 0;
	right = n-1;
	do {
		while(left	< right && memcmp(arr[left].value,pivot.value,20) <= 0 )	{
			left++;
		}
		while(right >= left && memcmp(arr[right].value,pivot.value,20) > 0)	{
			right--;
		}
		if(left < right)	{
			if(left == r || right == r)	{
				if(left == r)	{
					r = right;
				}
				if(right == r)	{
					r = left;
				}
			}
			_swap(&arr[right],&arr[left]);
		}
	}while(left < right);
	if(right != r)	{
		_swap(&arr[right],&arr[r]);
	}
	return right;
}

void _heapify(struct address_value *arr, int64_t n, int64_t i) {
	int64_t largest = i;
	int64_t l = 2 * i + 1;
	int64_t r = 2 * i + 2;
	if (l < n && memcmp(arr[l].value,arr[largest].value,20) > 0)
		largest = l;
	if (r < n && memcmp(arr[r].value,arr[largest].value,20) > 0)
		largest = r;
	if (largest != i) {
		_swap(&arr[i],&arr[largest]);
		_heapify(arr, n, largest);
	}
}

void _myheapsort(struct address_value	*arr, int64_t n)	{
	int64_t i;
	for ( i = (n / 2) - 1; i >=	0; i--)	{
		_heapify(arr, n, i);
	}
	for ( i = n - 1; i > 0; i--) {
		_swap(&arr[0] , &arr[i]);
		_heapify(arr, i, 0);
	}
}

/*	OK	*/
void bsgs_swap(struct bsgs_xvalue *a,struct bsgs_xvalue *b)	{
	struct bsgs_xvalue t;
	t	= *a;
	*a = *b;
	*b =	t;
}

/*	OK	*/
void bsgs_sort(struct bsgs_xvalue *arr,int64_t n)	{
	uint32_t depthLimit = ((uint32_t) ceil(log(n))) * 2;
	bsgs_introsort(arr,depthLimit,n);
}

/*	OK	*/
void bsgs_introsort(struct bsgs_xvalue *arr,uint32_t depthLimit, int64_t n) {
	int64_t p;
	if(n > 1)	{
		if(n <= 16) {
			bsgs_insertionsort(arr,n);
		}
		else	{
			if(depthLimit == 0) {
				bsgs_myheapsort(arr,n);
			}
			else	{
				p = bsgs_partition(arr,n);
				if(p > 0) bsgs_introsort(arr , depthLimit-1 , p);
				if(p < n) bsgs_introsort(&arr[p+1],depthLimit-1,n-(p+1));
			}
		}
	}
}

/*	OK	*/
void bsgs_insertionsort(struct bsgs_xvalue *arr, int64_t n) {
	int64_t j;
	int64_t i;
	struct bsgs_xvalue key;
	for(i = 1; i < n ; i++ ) {
		key = arr[i];
		j= i-1;
		while(j >= 0 && memcmp(arr[j].value,key.value,BSGS_XVALUE_RAM) > 0) {
			arr[j+1] = arr[j];
			j--;
		}
		arr[j+1] = key;
	}
}

int64_t bsgs_partition(struct bsgs_xvalue *arr, int64_t n)	{
	struct bsgs_xvalue pivot;
	int64_t r,left,right;
	char *hextemp;
	r = n/2;
	pivot = arr[r];
	left = 0;
	right = n-1;
	do {
		while(left	< right && memcmp(arr[left].value,pivot.value,BSGS_XVALUE_RAM) <= 0 )	{
			left++;
		}
		while(right >= left && memcmp(arr[right].value,pivot.value,BSGS_XVALUE_RAM) > 0)	{
			right--;
		}
		if(left < right)	{
			if(left == r || right == r)	{
				if(left == r)	{
					r = right;
				}
				if(right == r)	{
					r = left;
				}
			}
			bsgs_swap(&arr[right],&arr[left]);
		}
	}while(left < right);
	if(right != r)	{
		bsgs_swap(&arr[right],&arr[r]);
	}
	return right;
}

void bsgs_heapify(struct bsgs_xvalue *arr, int64_t n, int64_t i) {
	int64_t largest = i;
	int64_t l = 2 * i + 1;
	int64_t r = 2 * i + 2;
	if (l < n && memcmp(arr[l].value,arr[largest].value,BSGS_XVALUE_RAM) > 0)
		largest = l;
	if (r < n && memcmp(arr[r].value,arr[largest].value,BSGS_XVALUE_RAM) > 0)
		largest = r;
	if (largest != i) {
		bsgs_swap(&arr[i],&arr[largest]);
		bsgs_heapify(arr, n, largest);
	}
}

void bsgs_myheapsort(struct bsgs_xvalue	*arr, int64_t n)	{
	int64_t i;
	for ( i = (n / 2) - 1; i >=	0; i--)	{
		bsgs_heapify(arr, n, i);
	}
	for ( i = n - 1; i > 0; i--) {
		bsgs_swap(&arr[0] , &arr[i]);
		bsgs_heapify(arr, i, 0);
	}
}

int bsgs_searchbinary(struct bsgs_xvalue *buffer,char *data,int64_t array_length,uint64_t *r_value) {
	char *temp_read;
	int64_t min,max,half,current;
	int r = 0,rcmp;
	min = 0;
	current = 0;
	max = array_length;
	half = array_length;
	while(!r && half >= 1) {
		half = (max - min)/2;
		rcmp = memcmp(data+16,buffer[current+half].value,BSGS_XVALUE_RAM);
		if(rcmp == 0)	{
			*r_value = buffer[current+half].index;
			r = 1;
		}
		else	{
			if(rcmp < 0) {
				max = (max-half);
			}
			else	{
				min = (min+half);
			}
			current = min;
		}
	}
	return r;
}

#if defined(_WIN64) && !defined(__CYGWIN__)
DWORD WINAPI thread_process_bsgs(LPVOID vargp) {
#else
void *thread_process_bsgs(void *vargp)	{
#endif

	FILE *filekey;
	struct tothread *tt;
	char xpoint_raw[32],*aux_c,*hextemp;
	Int base_key,keyfound;
	Point base_point,point_aux,point_found;
	uint32_t i,j,k,l,r,salir,thread_number,flip_detector, cycles;
	
	IntGroup *grp = new IntGroup(CPU_GRP_SIZE / 2 + 1);
	Point startP;
	
	int hLength = (CPU_GRP_SIZE / 2 - 1);
	
	Int dx[CPU_GRP_SIZE / 2 + 1];
	Point pts[CPU_GRP_SIZE];

	Int dy;
	Int dyn;
	Int _s;
	Int _p;
	Int km,intaux;
	Point pp;
	Point pn;
	grp->Set(dx);

	
	tt = (struct tothread *)vargp;
	thread_number = tt->nt;
	free(tt);
	
	cycles = bsgs_aux / 1024;
	if(bsgs_aux % 1024 != 0)	{
		cycles++;
	}
	
	/*
		We do this in an atomic pthread_mutex operation to not affect others threads
		so BSGS_CURRENT is never the same between threads
	*/
#if defined(_WIN64) && !defined(__CYGWIN__)
	WaitForSingleObject(bsgs_thread, INFINITE);
#else
	pthread_mutex_lock(&bsgs_thread);
#endif
	
	base_key.Set(&BSGS_CURRENT);	/* we need to set our base_key to the current BSGS_CURRENT value*/
	BSGS_CURRENT.Add(&BSGS_N);	/*Then add BSGS_N to BSGS_CURRENT*/
#if defined(_WIN64) && !defined(__CYGWIN__)
	ReleaseMutex(bsgs_thread);
#else
	pthread_mutex_unlock(&bsgs_thread);
#endif
	
	intaux.Set(&BSGS_M);
	intaux.Mult(CPU_GRP_SIZE/2);
	
	flip_detector = FLIPBITLIMIT;
	//if(FLAGDEBUG)	{ printf("bsgs_aux: %lu\n",bsgs_aux);}

	/*
		while base_key is less than n_range_end then:
	*/
	while(base_key.IsLower(&n_range_end) )	{
		if(thread_number == 0 && flip_detector == 0)	{
			memorycheck_bsgs();
			flip_detector = FLIPBITLIMIT;
		}
		if(FLAGMATRIX)	{
			aux_c = base_key.GetBase16();
			printf("[+] Thread 0x%s \n",aux_c);
			fflush(stdout);
			free(aux_c);
		}
		else	{
			if(FLAGQUIET == 0){
				aux_c = base_key.GetBase16();
				printf("\r[+] Thread 0x%s   \r",aux_c);
				fflush(stdout);
				free(aux_c);
				THREADOUTPUT = 1;
			}
		}
		
		base_point = secp->ComputePublicKey(&base_key);

		km.Set(&base_key);
		km.Neg();
		
		km.Add(&secp->order);
		km.Sub(&intaux);
		point_aux = secp->ComputePublicKey(&km);
		
		for(k = 0; k < bsgs_point_number ; k++)	{
			if(bsgs_found[k] == 0)	{
				if(base_point.equals(OriginalPointsBSGS[k]))	{
					hextemp = base_key.GetBase16();
					printf("[+] Thread Key found privkey %s  \n",hextemp);
					aux_c = secp->GetPublicKeyHex(OriginalPointsBSGScompressed[k],base_point);
					printf("[+] Publickey %s\n",aux_c);
					
#if defined(_WIN64) && !defined(__CYGWIN__)
					WaitForSingleObject(write_keys, INFINITE);
#else
					pthread_mutex_lock(&write_keys);
#endif

					filekey = fopen("KEYFOUNDKEYFOUND.txt","a");
					if(filekey != NULL)	{
						fprintf(filekey,"Key found privkey %s\nPublickey %s\n",hextemp,aux_c);
						fclose(filekey);
					}
					free(hextemp);
					free(aux_c);
#if defined(_WIN64) && !defined(__CYGWIN__)
					ReleaseMutex(write_keys);
#else
					pthread_mutex_unlock(&write_keys);
#endif
					bsgs_found[k] = 1;
					salir = 1;
					for(l = 0; l < bsgs_point_number && salir; l++)	{
						salir &= bsgs_found[l];
					}
					if(salir)	{
						printf("All points were found\n");
						exit(0);
					}
				}
				else	{
					startP  = secp->AddDirect(OriginalPointsBSGS[k],point_aux);
					int j = 0;
					while( j < cycles && bsgs_found[k]== 0 )	{
					
						int i;
						
						for(i = 0; i < hLength; i++) {
							dx[i].ModSub(&GSn[i].x,&startP.x);
						}
						dx[i].ModSub(&GSn[i].x,&startP.x);  // For the first point
						dx[i+1].ModSub(&_2GSn.x,&startP.x); // For the next center point

						// Grouped ModInv
						grp->ModInv();
						
						/*
						We use the fact that P + i*G and P - i*G has the same deltax, so the same inverse
						We compute key in the positive and negative way from the center of the group
						*/

						// center point
						pts[CPU_GRP_SIZE / 2] = startP;
						
						for(i = 0; i<hLength; i++) {

							pp = startP;
							pn = startP;

							// P = startP + i*G
							dy.ModSub(&GSn[i].y,&pp.y);

							_s.ModMulK1(&dy,&dx[i]);        // s = (p2.y-p1.y)*inverse(p2.x-p1.x);
							_p.ModSquareK1(&_s);            // _p = pow2(s)

							pp.x.ModNeg();
							pp.x.ModAdd(&_p);
							pp.x.ModSub(&GSn[i].x);           // rx = pow2(s) - p1.x - p2.x;
							
#if 0
	  pp.y.ModSub(&GSn[i].x,&pp.x);
	  pp.y.ModMulK1(&_s);
	  pp.y.ModSub(&GSn[i].y);           // ry = - p2.y - s*(ret.x-p2.x);  
#endif

							// P = startP - i*G  , if (x,y) = i*G then (x,-y) = -i*G
							dyn.Set(&GSn[i].y);
							dyn.ModNeg();
							dyn.ModSub(&pn.y);

							_s.ModMulK1(&dyn,&dx[i]);       // s = (p2.y-p1.y)*inverse(p2.x-p1.x);
							_p.ModSquareK1(&_s);            // _p = pow2(s)

							pn.x.ModNeg();
							pn.x.ModAdd(&_p);
							pn.x.ModSub(&GSn[i].x);          // rx = pow2(s) - p1.x - p2.x;

#if 0
	  pn.y.ModSub(&GSn[i].x,&pn.x);
	  pn.y.ModMulK1(&_s);
	  pn.y.ModAdd(&GSn[i].y);          // ry = - p2.y - s*(ret.x-p2.x);  
#endif


							pts[CPU_GRP_SIZE / 2 + (i + 1)] = pp;
							pts[CPU_GRP_SIZE / 2 - (i + 1)] = pn;

						}

						// First point (startP - (GRP_SZIE/2)*G)
						pn = startP;
						dyn.Set(&GSn[i].y);
						dyn.ModNeg();
						dyn.ModSub(&pn.y);

						_s.ModMulK1(&dyn,&dx[i]);
						_p.ModSquareK1(&_s);

						pn.x.ModNeg();
						pn.x.ModAdd(&_p);
						pn.x.ModSub(&GSn[i].x);

#if 0
	pn.y.ModSub(&GSn[i].x,&pn.x);
	pn.y.ModMulK1(&_s);
	pn.y.ModAdd(&GSn[i].y);
#endif

						pts[0] = pn;
						
						for(int i = 0; i<CPU_GRP_SIZE && bsgs_found[k]== 0; i++) {
							pts[i].x.Get32Bytes((unsigned char*)xpoint_raw);
							/*
							if(FLAGDEBUG)	{
								hextemp = tohex(xpoint_raw,32);
								printf("Checking for : %s\nJ: %i\nI: %i\nK: %i\n",hextemp,j,i,k);
								free(hextemp);
							}
							*/
							r = bloom_check(&bloom_bP[((unsigned char)xpoint_raw[0])],xpoint_raw,32);
							if(r) {
								/*
								if(FLAGDEBUG)	{
									hextemp = tohex(xpoint_raw,32);
									printf("bloom_check OK : %s\nJ: %i\nI: %i\n",hextemp,j,i);
									free(hextemp);
								}
								*/
								r = bsgs_secondcheck(&base_key,((j*1024) + i),k,&keyfound);
								/*
								if(FLAGDEBUG)	{ printf(" ======= End Second check\n");}
								*/
								if(r)	{
									hextemp = keyfound.GetBase16();
									printf("[+] Thread Key found privkey %s   \n",hextemp);
									point_found = secp->ComputePublicKey(&keyfound);
									aux_c = secp->GetPublicKeyHex(OriginalPointsBSGScompressed[k],point_found);
									printf("[+] Publickey %s\n",aux_c);
#if defined(_WIN64) && !defined(__CYGWIN__)
									WaitForSingleObject(write_keys, INFINITE);
#else
									pthread_mutex_lock(&write_keys);
#endif

									filekey = fopen("KEYFOUNDKEYFOUND.txt","a");
									if(filekey != NULL)	{
										fprintf(filekey,"Key found privkey %s\nPublickey %s\n",hextemp,aux_c);
										fclose(filekey);
									}
									free(hextemp);
									free(aux_c);
#if defined(_WIN64) && !defined(__CYGWIN__)
					ReleaseMutex(write_keys);
#else
					pthread_mutex_unlock(&write_keys);
#endif
									bsgs_found[k] = 1;
									salir = 1;
									for(l = 0; l < bsgs_point_number && salir; l++)	{
										salir &= bsgs_found[l];
									}
									if(salir)	{
										printf("All points were found\n");
										exit(0);
									}
								} //End if second check
							}//End if first check
							
						}// For for pts variable
						
						// Next start point (startP += (bsSize*GRP_SIZE).G)
						
						pp = startP;
						dy.ModSub(&_2GSn.y,&pp.y);

						_s.ModMulK1(&dy,&dx[i + 1]);
						_p.ModSquareK1(&_s);

						pp.x.ModNeg();
						pp.x.ModAdd(&_p);
						pp.x.ModSub(&_2GSn.x);

						pp.y.ModSub(&_2GSn.x,&pp.x);
						pp.y.ModMulK1(&_s);
						pp.y.ModSub(&_2GSn.y);
						startP = pp;
						
						j++;
					} //while all the aMP points
				} // end else
			}// End if 
		}
		steps[thread_number]++;
#if defined(_WIN64) && !defined(__CYGWIN__)
		WaitForSingleObject(bsgs_thread, INFINITE);
#else
		pthread_mutex_lock(&bsgs_thread);
#endif

		base_key.Set(&BSGS_CURRENT);
		BSGS_CURRENT.Add(&BSGS_N);
#if defined(_WIN64) && !defined(__CYGWIN__)
		ReleaseMutex(bsgs_thread);
#else
		pthread_mutex_unlock(&bsgs_thread);
#endif

		flip_detector--;
	}
	ends[thread_number] = 1;
	return NULL;
}

#if defined(_WIN64) && !defined(__CYGWIN__)
DWORD WINAPI thread_process_bsgs_random(LPVOID vargp) {
#else
void *thread_process_bsgs_random(void *vargp)	{
#endif

	FILE *filekey;
	struct tothread *tt;
	char xpoint_raw[32],*aux_c,*hextemp;
	Int base_key,keyfound,n_range_random;
	Point base_point,point_aux,point_found;
	uint32_t i,j,l,k,r,salir,thread_number,flip_detector,cycles;
	
	IntGroup *grp = new IntGroup(CPU_GRP_SIZE / 2 + 1);
	Point startP;
	
	int hLength = (CPU_GRP_SIZE / 2 - 1);
	
	Int dx[CPU_GRP_SIZE / 2 + 1];
	Point pts[CPU_GRP_SIZE];

	Int dy;
	Int dyn;
	Int _s;
	Int _p;
	Int km,intaux;
	Point pp;
	Point pn;
	grp->Set(dx);


	tt = (struct tothread *)vargp;
	thread_number = tt->nt;
	free(tt);
	
	cycles = bsgs_aux / 1024;
	if(bsgs_aux % 1024 != 0)	{
		cycles++;
	}
	
	/*          | Start Range	| End Range     |
		None	| 1             | EC.N          |
		-b	bit | Min bit value | Max bit value |
		-r	A:B | A             | B             |
	*/
#if defined(_WIN64) && !defined(__CYGWIN__)
	WaitForSingleObject(bsgs_thread, INFINITE);
#else
	pthread_mutex_lock(&bsgs_thread);
#endif

	base_key.Rand(&n_range_start,&n_range_end);
#if defined(_WIN64) && !defined(__CYGWIN__)
	ReleaseMutex(bsgs_thread);
#else
	pthread_mutex_unlock(&bsgs_thread);
#endif
	intaux.Set(&BSGS_M);
	intaux.Mult(CPU_GRP_SIZE/2);
	flip_detector = FLIPBITLIMIT;
	/*
		while base_key is less than n_range_end then:
	*/
	while(base_key.IsLower(&n_range_end))	{
		if(thread_number == 0 && flip_detector == 0)	{
			memorycheck_bsgs();
			flip_detector = FLIPBITLIMIT;
		}
		if(FLAGMATRIX)	{
				aux_c = base_key.GetBase16();
				printf("[+] Thread 0x%s  \n",aux_c);
				fflush(stdout);
				free(aux_c);
		}
		else{
			if(FLAGQUIET == 0){
				aux_c = base_key.GetBase16();
				printf("\r[+] Thread 0x%s  \r",aux_c);
				fflush(stdout);
				free(aux_c);
				THREADOUTPUT = 1;
			}
		}
		base_point = secp->ComputePublicKey(&base_key);

		km.Set(&base_key);
		km.Neg();
		
		
		km.Add(&secp->order);
		km.Sub(&intaux);
		point_aux = secp->ComputePublicKey(&km);


		/* We need to test individually every point in BSGS_Q */
		for(k = 0; k < bsgs_point_number ; k++)	{
			if(bsgs_found[k] == 0)	{
				if(base_point.equals(OriginalPointsBSGS[k]))	{
					hextemp = base_key.GetBase16();
					printf("[+] Thread Key found privkey %s     \n",hextemp);
					aux_c = secp->GetPublicKeyHex(OriginalPointsBSGScompressed[k],base_point);
					printf("[+] Publickey %s\n",aux_c);
					
#if defined(_WIN64) && !defined(__CYGWIN__)
					WaitForSingleObject(write_keys, INFINITE);
#else
					pthread_mutex_lock(&write_keys);
#endif

					filekey = fopen("KEYFOUNDKEYFOUND.txt","a");
					if(filekey != NULL)	{
						fprintf(filekey,"Key found privkey %s\nPublickey %s\n",hextemp,aux_c);
						fclose(filekey);
					}
					free(hextemp);
					free(aux_c);
#if defined(_WIN64) && !defined(__CYGWIN__)
					ReleaseMutex(write_keys);
#else
					pthread_mutex_unlock(&write_keys);
#endif

					
					bsgs_found[k] = 1;
					salir = 1;
					for(l = 0; l < bsgs_point_number && salir; l++)	{
						salir &= bsgs_found[l];
					}
					if(salir)	{
						printf("All points were found\n");
						exit(0);
					}
				}
				else	{
				
					startP  = secp->AddDirect(OriginalPointsBSGS[k],point_aux);
					int j = 0;
					while( j < cycles && bsgs_found[k]== 0 )	{
					
						int i;
						for(i = 0; i < hLength; i++) {
							dx[i].ModSub(&GSn[i].x,&startP.x);
						}
						dx[i].ModSub(&GSn[i].x,&startP.x);  // For the first point
						dx[i+1].ModSub(&_2GSn.x,&startP.x); // For the next center point

						// Grouped ModInv
						grp->ModInv();
						
						/*
						We use the fact that P + i*G and P - i*G has the same deltax, so the same inverse
						We compute key in the positive and negative way from the center of the group
						*/

						// center point
						pts[CPU_GRP_SIZE / 2] = startP;
						
						for(i = 0; i<hLength; i++) {

							pp = startP;
							pn = startP;

							// P = startP + i*G
							dy.ModSub(&GSn[i].y,&pp.y);

							_s.ModMulK1(&dy,&dx[i]);        // s = (p2.y-p1.y)*inverse(p2.x-p1.x);
							_p.ModSquareK1(&_s);            // _p = pow2(s)

							pp.x.ModNeg();
							pp.x.ModAdd(&_p);
							pp.x.ModSub(&GSn[i].x);           // rx = pow2(s) - p1.x - p2.x;
							
#if 0
	  pp.y.ModSub(&GSn[i].x,&pp.x);
	  pp.y.ModMulK1(&_s);
	  pp.y.ModSub(&GSn[i].y);           // ry = - p2.y - s*(ret.x-p2.x);  
#endif

							// P = startP - i*G  , if (x,y) = i*G then (x,-y) = -i*G
							dyn.Set(&GSn[i].y);
							dyn.ModNeg();
							dyn.ModSub(&pn.y);

							_s.ModMulK1(&dyn,&dx[i]);       // s = (p2.y-p1.y)*inverse(p2.x-p1.x);
							_p.ModSquareK1(&_s);            // _p = pow2(s)

							pn.x.ModNeg();
							pn.x.ModAdd(&_p);
							pn.x.ModSub(&GSn[i].x);          // rx = pow2(s) - p1.x - p2.x;

#if 0
	  pn.y.ModSub(&GSn[i].x,&pn.x);
	  pn.y.ModMulK1(&_s);
	  pn.y.ModAdd(&GSn[i].y);          // ry = - p2.y - s*(ret.x-p2.x);  
#endif


							pts[CPU_GRP_SIZE / 2 + (i + 1)] = pp;
							pts[CPU_GRP_SIZE / 2 - (i + 1)] = pn;

						}

						// First point (startP - (GRP_SZIE/2)*G)
						pn = startP;
						dyn.Set(&GSn[i].y);
						dyn.ModNeg();
						dyn.ModSub(&pn.y);

						_s.ModMulK1(&dyn,&dx[i]);
						_p.ModSquareK1(&_s);

						pn.x.ModNeg();
						pn.x.ModAdd(&_p);
						pn.x.ModSub(&GSn[i].x);

#if 0
	pn.y.ModSub(&GSn[i].x,&pn.x);
	pn.y.ModMulK1(&_s);
	pn.y.ModAdd(&GSn[i].y);
#endif

						pts[0] = pn;
						
						for(int i = 0; i<CPU_GRP_SIZE && bsgs_found[k]== 0; i++) {
							pts[i].x.Get32Bytes((unsigned char*)xpoint_raw);
							r = bloom_check(&bloom_bP[((unsigned char)xpoint_raw[0])],xpoint_raw,32);
							if(r) {
								r = bsgs_secondcheck(&base_key,((j*1024) + i),k,&keyfound);
								if(r)	{
									hextemp = keyfound.GetBase16();
									printf("[+] Thread Key found privkey %s    \n",hextemp);
									point_found = secp->ComputePublicKey(&keyfound);
									aux_c = secp->GetPublicKeyHex(OriginalPointsBSGScompressed[k],point_found);
									printf("[+] Publickey %s\n",aux_c);
#if defined(_WIN64) && !defined(__CYGWIN__)
									WaitForSingleObject(write_keys, INFINITE);
#else
									pthread_mutex_lock(&write_keys);
#endif

									filekey = fopen("KEYFOUNDKEYFOUND.txt","a");
									if(filekey != NULL)	{
										fprintf(filekey,"Key found privkey %s\nPublickey %s\n",hextemp,aux_c);
										fclose(filekey);
									}
									free(hextemp);
									free(aux_c);
#if defined(_WIN64) && !defined(__CYGWIN__)
									ReleaseMutex(write_keys);
#else
									pthread_mutex_unlock(&write_keys);
#endif

									bsgs_found[k] = 1;
									salir = 1;
									for(l = 0; l < bsgs_point_number && salir; l++)	{
										salir &= bsgs_found[l];
									}
									if(salir)	{
										printf("All points were found\n");
										exit(0);
									}
								} //End if second check
							}//End if first check
							
						}// For for pts variable
						
						// Next start point (startP += (bsSize*GRP_SIZE).G)
						
						pp = startP;
						dy.ModSub(&_2GSn.y,&pp.y);

						_s.ModMulK1(&dy,&dx[i + 1]);
						_p.ModSquareK1(&_s);

						pp.x.ModNeg();
						pp.x.ModAdd(&_p);
						pp.x.ModSub(&_2GSn.x);

						pp.y.ModSub(&_2GSn.x,&pp.x);
						pp.y.ModMulK1(&_s);
						pp.y.ModSub(&_2GSn.y);
						startP = pp;
						
						j++;
						
					}	//End While

					
					
				} //End else

				
			}	//End if
		} // End for with k bsgs_point_number

		steps[thread_number]++;
#if defined(_WIN64) && !defined(__CYGWIN__)
		WaitForSingleObject(bsgs_thread, INFINITE);
		base_key.Rand(&n_range_start,&n_range_end);
		ReleaseMutex(bsgs_thread);
#else
		pthread_mutex_lock(&bsgs_thread);
		base_key.Rand(&n_range_start,&n_range_end);
		pthread_mutex_unlock(&bsgs_thread);
#endif
		flip_detector--;
	}
	ends[thread_number] = 1;
	return NULL;
}

/*
	The bsgs_secondcheck function is made to perform a second BSGS search in a Range of less size.
	This funtion is made with the especific purpouse to USE a smaller bPtable in RAM.
*/
int bsgs_secondcheck(Int *start_range,uint32_t a,uint32_t k_index,Int *privatekey)	{
	uint64_t j = 0;
	int i = 0,found = 0,r = 0;
	Int base_key;
	Point base_point,point_aux;
	Point BSGS_Q, BSGS_S,BSGS_Q_AMP;
	char xpoint_raw[32],*hextemp;

	base_key.Set(&BSGS_M);
	base_key.Mult((uint64_t) a);
	base_key.Add(start_range);

	base_point = secp->ComputePublicKey(&base_key);
	point_aux = secp->Negation(base_point);


	BSGS_S = secp->AddDirect(OriginalPointsBSGS[k_index],point_aux);
	BSGS_Q.Set(BSGS_S);
	do {
		BSGS_S.x.Get32Bytes((unsigned char *) xpoint_raw);
		r = bloom_check(&bloom_bPx2nd[(uint8_t) xpoint_raw[0]],xpoint_raw,32);
		if(r)	{
			found = bsgs_thirdcheck(&base_key,i,k_index,privatekey);
		}
		BSGS_Q_AMP = secp->AddDirect(BSGS_Q,BSGS_AMP2[i]);
		BSGS_S.Set(BSGS_Q_AMP);
		i++;
	}while(i < 32 && !found);
	return found;
}

int bsgs_thirdcheck(Int *start_range,uint32_t a,uint32_t k_index,Int *privatekey)	{
	uint64_t j = 0;
	int i = 0,found = 0,r = 0;
	Int base_key;
	Point base_point,point_aux;
	Point BSGS_Q, BSGS_S,BSGS_Q_AMP;
	char xpoint_raw[32],*hextemp,*hextemp2;

	base_key.Set(&BSGS_M2);
	base_key.Mult((uint64_t) a);
	base_key.Add(start_range);

	base_point = secp->ComputePublicKey(&base_key);
	point_aux = secp->Negation(base_point);
	
	/*
	if(FLAGDEBUG) { printf("===== Function  bsgs_thirdcheck\n");}
	if(FLAGDEBUG)	{
		hextemp2 = start_range->GetBase16();
		hextemp = base_key.GetBase16();
		printf("[D] %s , %i , %i\n",hextemp,a,k_index);
		free(hextemp);
		free(hextemp2);
	}
	*/
	
	BSGS_S = secp->AddDirect(OriginalPointsBSGS[k_index],point_aux);
	BSGS_Q.Set(BSGS_S);
	
	do {
		BSGS_S.x.Get32Bytes((unsigned char *)xpoint_raw);
		r = bloom_check(&bloom_bPx3rd[(uint8_t)xpoint_raw[0]],xpoint_raw,32);
		/*
		if(FLAGDEBUG) {  
			hextemp = tohex(xpoint_raw,32);
			printf("hextemp: %s : r = %i\n",hextemp,r);
			free(hextemp);
		}
		*/
		if(r)	{
			r = bsgs_searchbinary(bPtable,xpoint_raw,bsgs_m3,&j);
			if(r)	{
				privatekey->Set(&BSGS_M3);
				privatekey->Mult((uint64_t)i);
				privatekey->Add((uint64_t)(j+1));
				privatekey->Add(&base_key);
				point_aux = secp->ComputePublicKey(privatekey);
				if(point_aux.x.IsEqual(&OriginalPointsBSGS[k_index].x))	{
					found = 1;
				}
				else	{
					privatekey->Set(&BSGS_M3);
					privatekey->Mult((uint64_t)i);
					privatekey->Sub((uint64_t)(j+1));
					privatekey->Add(&base_key);
					point_aux = secp->ComputePublicKey(privatekey);
					if(point_aux.x.IsEqual(&OriginalPointsBSGS[k_index].x))	{
						found = 1;
					}
				}
			}
		}
		BSGS_Q_AMP = secp->AddDirect(BSGS_Q,BSGS_AMP3[i]);
		BSGS_S.Set(BSGS_Q_AMP);
		i++;
	}while(i < 32 && !found);
	return found;
}


void sleep_ms(int milliseconds)	{ // cross-platform sleep function
#if defined(_WIN64) && !defined(__CYGWIN__)
    Sleep(milliseconds);
#elif _POSIX_C_SOURCE >= 199309L
    struct timespec ts;
    ts.tv_sec = milliseconds / 1000;
    ts.tv_nsec = (milliseconds % 1000) * 1000000;
    nanosleep(&ts, NULL);
#else
    if (milliseconds >= 1000)
      sleep(milliseconds / 1000);
    usleep((milliseconds % 1000) * 1000);
#endif
}

#if defined(_WIN64) && !defined(__CYGWIN__)
DWORD WINAPI thread_pub2rmd(LPVOID vargp) {
#else
void *thread_pub2rmd(void *vargp)	{
#endif
	FILE *fd;
	Int key_mpz;
	struct tothread *tt;
	uint64_t i,limit,j;
	char digest160[20];
	char digest256[32];
	char *temphex;
	int thread_number,r;
	int pub2rmd_continue = 1;
	struct publickey pub;
	limit = 0xFFFFFFFF;
	tt = (struct tothread *)vargp;
	thread_number = tt->nt;
	do {
		if(FLAGRANDOM){
			key_mpz.Rand(&n_range_start,&n_range_diff);
		}
		else	{
			if(n_range_start.IsLower(&n_range_end))	{
#if defined(_WIN64) && !defined(__CYGWIN__)
				WaitForSingleObject(write_random, INFINITE);
				key_mpz.Set(&n_range_start);
				n_range_start.Add(N_SECUENTIAL_MAX);
				ReleaseMutex(write_random);
#else
				pthread_mutex_lock(&write_random);
				key_mpz.Set(&n_range_start);
				n_range_start.Add(N_SECUENTIAL_MAX);
				pthread_mutex_lock(&write_random);
#endif
			}
			else	{
				pub2rmd_continue = 0;
			}
		}
		if(pub2rmd_continue)	{
			key_mpz.Get32Bytes(pub.X.data8);
			pub.parity = 0x02;
			pub.X.data32[7] = 0;
			if(FLAGMATRIX)	{
				temphex = tohex((char*)&pub,33);
				printf("[+] Thread 0x%s  \n",temphex);
				free(temphex);
				fflush(stdout);
			}
			else	{
				if(FLAGQUIET == 0)	{
					temphex = tohex((char*)&pub,33);
					printf("\r[+] Thread %s  \r",temphex);
					free(temphex);
					fflush(stdout);
					THREADOUTPUT = 1;
				}
			}
			for(i = 0 ; i < limit ; i++) {
				pub.parity = 0x02;
				sha256((uint8_t*)&pub, 33, (uint8_t*)digest256);
				RMD160Data((const unsigned char*)digest256,32, digest160);
				r = bloom_check(&bloom,digest160,MAXLENGTHADDRESS);
				if(r)  {
						r = searchbinary(addressTable,digest160,N);
						if(r)	{
							temphex = tohex((char*)&pub,33);
							printf("\nHit: Publickey found %s\n",temphex);
							fd = fopen("KEYFOUNDKEYFOUND.txt","a+");
							if(fd != NULL)	{
#if defined(_WIN64) && !defined(__CYGWIN__)
								WaitForSingleObject(write_keys, INFINITE);
								fprintf(fd,"Publickey found %s\n",temphex);
								fclose(fd);
								ReleaseMutex(write_keys);
#else
								pthread_mutex_lock(&write_keys);
								fprintf(fd,"Publickey found %s\n",temphex);
								fclose(fd);
								pthread_mutex_unlock(&write_keys);
#endif
							}
							else	{
								fprintf(stderr,"\nPublickey found %s\nbut the file can't be open\n",temphex);
								exit(0);
							}
							free(temphex);
						}
				}
				pub.parity = 0x03;
				sha256((uint8_t*)&pub, 33,(uint8_t*) digest256);
				RMD160Data((const unsigned char*)digest256,32, digest160);
				r = bloom_check(&bloom,digest160,MAXLENGTHADDRESS);
				if(r)  {
					r = searchbinary(addressTable,digest160,N);
					if(r)  {
						temphex = tohex((char*)&pub,33);
						printf("\nHit: Publickey found %s\n",temphex);
						fd = fopen("KEYFOUNDKEYFOUND.txt","a+");
						if(fd != NULL)	{
#if defined(_WIN64) && !defined(__CYGWIN__)
							WaitForSingleObject(write_keys, INFINITE);
							fprintf(fd,"Publickey found %s\n",temphex);
							fclose(fd);
							ReleaseMutex(write_keys);

#else
							pthread_mutex_lock(&write_keys);
							fprintf(fd,"Publickey found %s\n",temphex);
							fclose(fd);
							pthread_mutex_unlock(&write_keys);
#endif
						}
						else	{
							fprintf(stderr,"\nPublickey found %s\nbut the file can't be open\n",temphex);
							exit(0);
						}
						free(temphex);
					}
				}
				pub.X.data32[7]++;
				if(pub.X.data32[7] % DEBUGCOUNT == 0)  {
					steps[thread_number]++;
				}
			}	
		}	
	}while(pub2rmd_continue);
	ends[thread_number] = 1;
	return NULL;
}

void init_generator()	{
	Point G = secp->ComputePublicKey(&stride);
	Point g;
	g.Set(G);
	Gn.reserve(CPU_GRP_SIZE / 2);
	Gn[0] = g;
	g = secp->DoubleDirect(g);
	Gn[1] = g;
	for(int i = 2; i < CPU_GRP_SIZE / 2; i++) {
		g = secp->AddDirect(g,G);
		Gn[i] = g;
	}
	_2Gn = secp->DoubleDirect(Gn[CPU_GRP_SIZE / 2 - 1]);
}

#if defined(_WIN64) && !defined(__CYGWIN__)
DWORD WINAPI thread_bPload(LPVOID vargp) {
#else
void *thread_bPload(void *vargp)	{
#endif

	char rawvalue[32],hexraw[65];
	struct bPload *tt;
	uint64_t i_counter,i,j,nbStep,to;
	
	IntGroup *grp = new IntGroup(CPU_GRP_SIZE / 2 + 1);
	Point startP;
	Int dx[CPU_GRP_SIZE / 2 + 1];
	Point pts[CPU_GRP_SIZE];
	Int dy,dyn,_s,_p;
	Point pp,pn;
	
	int bloom_bP_index,hLength = (CPU_GRP_SIZE / 2 - 1) ,threadid;
	tt = (struct bPload *)vargp;
	Int km((uint64_t)(tt->from + 1));
	threadid = tt->threadid;
	//if(FLAGDEBUG) printf("[D] thread %i from %" PRIu64 " to %" PRIu64 "\n",threadid,tt->from,tt->to);
	
	i_counter = tt->from;

	nbStep = (tt->to - tt->from) / CPU_GRP_SIZE;
	
	if( ((tt->to - tt->from) % CPU_GRP_SIZE )  != 0)	{
		nbStep++;
	}
	//if(FLAGDEBUG) printf("[D] thread %i nbStep %" PRIu64 "\n",threadid,nbStep);
	to = tt->to;
	
	km.Add((uint64_t)(CPU_GRP_SIZE / 2));
	startP = secp->ComputePublicKey(&km);
	grp->Set(dx);
	for(uint64_t s=0;s<nbStep;s++) {
		for(i = 0; i < hLength; i++) {
			dx[i].ModSub(&Gn[i].x,&startP.x);
		}
		dx[i].ModSub(&Gn[i].x,&startP.x); // For the first point
		dx[i + 1].ModSub(&_2Gn.x,&startP.x);// For the next center point
		// Grouped ModInv
		grp->ModInv();

		// We use the fact that P + i*G and P - i*G has the same deltax, so the same inverse
		// We compute key in the positive and negative way from the center of the group
		// center point
		
		pts[CPU_GRP_SIZE / 2] = startP;	//Center point

		for(i = 0; i<hLength; i++) {
			pp = startP;
			pn = startP;

			// P = startP + i*G
			dy.ModSub(&Gn[i].y,&pp.y);

			_s.ModMulK1(&dy,&dx[i]);        // s = (p2.y-p1.y)*inverse(p2.x-p1.x);
			_p.ModSquareK1(&_s);            // _p = pow2(s)

			pp.x.ModNeg();
			pp.x.ModAdd(&_p);
			pp.x.ModSub(&Gn[i].x);           // rx = pow2(s) - p1.x - p2.x;

#if 0
			pp.y.ModSub(&Gn[i].x,&pp.x);
			pp.y.ModMulK1(&_s);
			pp.y.ModSub(&Gn[i].y);           // ry = - p2.y - s*(ret.x-p2.x);
#endif

			// P = startP - i*G  , if (x,y) = i*G then (x,-y) = -i*G
			dyn.Set(&Gn[i].y);
			dyn.ModNeg();
			dyn.ModSub(&pn.y);

			_s.ModMulK1(&dyn,&dx[i]);      // s = (p2.y-p1.y)*inverse(p2.x-p1.x);
			_p.ModSquareK1(&_s);            // _p = pow2(s)

			pn.x.ModNeg();
			pn.x.ModAdd(&_p);
			pn.x.ModSub(&Gn[i].x);          // rx = pow2(s) - p1.x - p2.x;

#if 0
			pn.y.ModSub(&Gn[i].x,&pn.x);
			pn.y.ModMulK1(&_s);
			pn.y.ModAdd(&Gn[i].y);          // ry = - p2.y - s*(ret.x-p2.x);
#endif

			pts[CPU_GRP_SIZE / 2 + (i + 1)] = pp;
			pts[CPU_GRP_SIZE / 2 - (i + 1)] = pn;
		}

		// First point (startP - (GRP_SZIE/2)*G)
		pn = startP;
		dyn.Set(&Gn[i].y);
		dyn.ModNeg();
		dyn.ModSub(&pn.y);

		_s.ModMulK1(&dyn,&dx[i]);
		_p.ModSquareK1(&_s);

		pn.x.ModNeg();
		pn.x.ModAdd(&_p);
		pn.x.ModSub(&Gn[i].x);

#if 0
		pn.y.ModSub(&Gn[i].x,&pn.x);
		pn.y.ModMulK1(&_s);
		pn.y.ModAdd(&Gn[i].y);
#endif

		pts[0] = pn;
		for(j=0;j<CPU_GRP_SIZE;j++)	{
			pts[j].x.Get32Bytes((unsigned char*)rawvalue);
			bloom_bP_index = (uint8_t)rawvalue[0];
			/*
			if(FLAGDEBUG){
				tohex_dst(rawvalue,32,hexraw);
				printf("%i : %s : %i\n",i_counter,hexraw,bloom_bP_index);
			}
			*/
			if(i_counter < bsgs_m3)	{
				if(!FLAGREADEDFILE3)	{
					memcpy(bPtable[i_counter].value,rawvalue+16,BSGS_XVALUE_RAM);
					bPtable[i_counter].index = i_counter;
				}
				if(!FLAGREADEDFILE4)	{
#if defined(_WIN64) && !defined(__CYGWIN__)
					WaitForSingleObject(bloom_bPx3rd_mutex[bloom_bP_index], INFINITE);
					bloom_add(&bloom_bPx3rd[bloom_bP_index], rawvalue, BSGS_BUFFERXPOINTLENGTH);
					ReleaseMutex(bloom_bPx3rd_mutex[bloom_bP_index]);
#else
					pthread_mutex_lock(&bloom_bPx3rd_mutex[bloom_bP_index]);
					bloom_add(&bloom_bPx3rd[bloom_bP_index], rawvalue, BSGS_BUFFERXPOINTLENGTH);
					pthread_mutex_unlock(&bloom_bPx3rd_mutex[bloom_bP_index]);
#endif
				}
			}
			if(i_counter < bsgs_m2 && !FLAGREADEDFILE2)	{
#if defined(_WIN64) && !defined(__CYGWIN__)
				WaitForSingleObject(bloom_bPx2nd_mutex[bloom_bP_index], INFINITE);
				bloom_add(&bloom_bPx2nd[bloom_bP_index], rawvalue, BSGS_BUFFERXPOINTLENGTH);
				ReleaseMutex(bloom_bPx2nd_mutex[bloom_bP_index]);
#else
				pthread_mutex_lock(&bloom_bPx2nd_mutex[bloom_bP_index]);
				bloom_add(&bloom_bPx2nd[bloom_bP_index], rawvalue, BSGS_BUFFERXPOINTLENGTH);
				pthread_mutex_unlock(&bloom_bPx2nd_mutex[bloom_bP_index]);
#endif	
			}
			if(i_counter < to && !FLAGREADEDFILE1 )	{
#if defined(_WIN64) && !defined(__CYGWIN__)
				WaitForSingleObject(bloom_bP_mutex[bloom_bP_index], INFINITE);
				bloom_add(&bloom_bP[bloom_bP_index], rawvalue ,BSGS_BUFFERXPOINTLENGTH);
				ReleaseMutex(bloom_bP_mutex[bloom_bP_index);
#else
				pthread_mutex_lock(&bloom_bP_mutex[bloom_bP_index]);
				bloom_add(&bloom_bP[bloom_bP_index], rawvalue ,BSGS_BUFFERXPOINTLENGTH);
				pthread_mutex_unlock(&bloom_bP_mutex[bloom_bP_index]);
#endif
			}
			i_counter++;
		}
		// Next start point (startP + GRP_SIZE*G)
		pp = startP;
		dy.ModSub(&_2Gn.y,&pp.y);

		_s.ModMulK1(&dy,&dx[i + 1]);
		_p.ModSquareK1(&_s);

		pp.x.ModNeg();
		pp.x.ModAdd(&_p);
		pp.x.ModSub(&_2Gn.x);

		pp.y.ModSub(&_2Gn.x,&pp.x);
		pp.y.ModMulK1(&_s);
		pp.y.ModSub(&_2Gn.y);
		startP = pp;
	}
	delete grp;
#if defined(_WIN64) && !defined(__CYGWIN__)
	WaitForSingleObject(bPload_mutex[threadid], INFINITE);
	tt->finished = 1;
	ReleaseMutex(bPload_mutex[threadid]);
#else	
	pthread_mutex_lock(&bPload_mutex[threadid]);
	tt->finished = 1;
	pthread_mutex_unlock(&bPload_mutex[threadid]);
	pthread_exit(NULL);
#endif
	return NULL;
}

#if defined(_WIN64) && !defined(__CYGWIN__)
DWORD WINAPI thread_bPload_2blooms(LPVOID vargp) {
#else
void *thread_bPload_2blooms(void *vargp)	{
#endif
	char rawvalue[32];
	struct bPload *tt;
	uint64_t i_counter,i,j,nbStep,to;
	IntGroup *grp = new IntGroup(CPU_GRP_SIZE / 2 + 1);
	Point startP;
	Int dx[CPU_GRP_SIZE / 2 + 1];
	Point pts[CPU_GRP_SIZE];
	Int dy,dyn,_s,_p;
	Point pp,pn;
	int bloom_bP_index,hLength = (CPU_GRP_SIZE / 2 - 1) ,threadid;
	tt = (struct bPload *)vargp;
	Int km((uint64_t)(tt->from +1 ));
	threadid = tt->threadid;
	
	i_counter = tt->from;

	nbStep = (tt->to - (tt->from)) / CPU_GRP_SIZE;
	
	if( ((tt->to - (tt->from)) % CPU_GRP_SIZE )  != 0)	{
		nbStep++;
	}
	//if(FLAGDEBUG) printf("[D] thread %i nbStep %" PRIu64 "\n",threadid,nbStep);
	to = tt->to;
	
	km.Add((uint64_t)(CPU_GRP_SIZE / 2));
	startP = secp->ComputePublicKey(&km);
	grp->Set(dx);
	for(uint64_t s=0;s<nbStep;s++) {
		for(i = 0; i < hLength; i++) {
			dx[i].ModSub(&Gn[i].x,&startP.x);
		}
		dx[i].ModSub(&Gn[i].x,&startP.x); // For the first point
		dx[i + 1].ModSub(&_2Gn.x,&startP.x);// For the next center point
		// Grouped ModInv
		grp->ModInv();

		// We use the fact that P + i*G and P - i*G has the same deltax, so the same inverse
		// We compute key in the positive and negative way from the center of the group
		// center point
		
		pts[CPU_GRP_SIZE / 2] = startP;	//Center point

		for(i = 0; i<hLength; i++) {
			pp = startP;
			pn = startP;

			// P = startP + i*G
			dy.ModSub(&Gn[i].y,&pp.y);

			_s.ModMulK1(&dy,&dx[i]);        // s = (p2.y-p1.y)*inverse(p2.x-p1.x);
			_p.ModSquareK1(&_s);            // _p = pow2(s)

			pp.x.ModNeg();
			pp.x.ModAdd(&_p);
			pp.x.ModSub(&Gn[i].x);           // rx = pow2(s) - p1.x - p2.x;

#if 0
			pp.y.ModSub(&Gn[i].x,&pp.x);
			pp.y.ModMulK1(&_s);
			pp.y.ModSub(&Gn[i].y);           // ry = - p2.y - s*(ret.x-p2.x);
#endif

			// P = startP - i*G  , if (x,y) = i*G then (x,-y) = -i*G
			dyn.Set(&Gn[i].y);
			dyn.ModNeg();
			dyn.ModSub(&pn.y);

			_s.ModMulK1(&dyn,&dx[i]);      // s = (p2.y-p1.y)*inverse(p2.x-p1.x);
			_p.ModSquareK1(&_s);            // _p = pow2(s)

			pn.x.ModNeg();
			pn.x.ModAdd(&_p);
			pn.x.ModSub(&Gn[i].x);          // rx = pow2(s) - p1.x - p2.x;

#if 0
			pn.y.ModSub(&Gn[i].x,&pn.x);
			pn.y.ModMulK1(&_s);
			pn.y.ModAdd(&Gn[i].y);          // ry = - p2.y - s*(ret.x-p2.x);
#endif

			pts[CPU_GRP_SIZE / 2 + (i + 1)] = pp;
			pts[CPU_GRP_SIZE / 2 - (i + 1)] = pn;
		}

		// First point (startP - (GRP_SZIE/2)*G)
		pn = startP;
		dyn.Set(&Gn[i].y);
		dyn.ModNeg();
		dyn.ModSub(&pn.y);

		_s.ModMulK1(&dyn,&dx[i]);
		_p.ModSquareK1(&_s);

		pn.x.ModNeg();
		pn.x.ModAdd(&_p);
		pn.x.ModSub(&Gn[i].x);

#if 0
		pn.y.ModSub(&Gn[i].x,&pn.x);
		pn.y.ModMulK1(&_s);
		pn.y.ModAdd(&Gn[i].y);
#endif

		pts[0] = pn;
		for(j=0;j<CPU_GRP_SIZE;j++)	{
			pts[j].x.Get32Bytes((unsigned char*)rawvalue);
			bloom_bP_index = (uint8_t)rawvalue[0];
			if(i_counter < bsgs_m3)	{
				if(!FLAGREADEDFILE3)	{
					memcpy(bPtable[i_counter].value,rawvalue+16,BSGS_XVALUE_RAM);
					bPtable[i_counter].index = i_counter;
				}
				if(!FLAGREADEDFILE4)	{
#if defined(_WIN64) && !defined(__CYGWIN__)
					WaitForSingleObject(bloom_bPx3rd_mutex[bloom_bP_index], INFINITE);
					bloom_add(&bloom_bPx3rd[bloom_bP_index], rawvalue, BSGS_BUFFERXPOINTLENGTH);
					ReleaseMutex(bloom_bPx3rd_mutex[bloom_bP_index]);
#else
					pthread_mutex_lock(&bloom_bPx3rd_mutex[bloom_bP_index]);
					bloom_add(&bloom_bPx3rd[bloom_bP_index], rawvalue, BSGS_BUFFERXPOINTLENGTH);
					pthread_mutex_unlock(&bloom_bPx3rd_mutex[bloom_bP_index]);
#endif
				}
			}
			if(i_counter < bsgs_m2 && !FLAGREADEDFILE2)	{
#if defined(_WIN64) && !defined(__CYGWIN__)
					WaitForSingleObject(bloom_bPx2nd_mutex[bloom_bP_index], INFINITE);
					bloom_add(&bloom_bPx2nd[bloom_bP_index], rawvalue, BSGS_BUFFERXPOINTLENGTH);
					ReleaseMutex(bloom_bPx2nd_mutex[bloom_bP_index]);
#else
					pthread_mutex_lock(&bloom_bPx2nd_mutex[bloom_bP_index]);
					bloom_add(&bloom_bPx2nd[bloom_bP_index], rawvalue, BSGS_BUFFERXPOINTLENGTH);
					pthread_mutex_unlock(&bloom_bPx2nd_mutex[bloom_bP_index]);
#endif			
			}
			i_counter++;
		}
		// Next start point (startP + GRP_SIZE*G)
		pp = startP;
		dy.ModSub(&_2Gn.y,&pp.y);

		_s.ModMulK1(&dy,&dx[i + 1]);
		_p.ModSquareK1(&_s);

		pp.x.ModNeg();
		pp.x.ModAdd(&_p);
		pp.x.ModSub(&_2Gn.x);

		pp.y.ModSub(&_2Gn.x,&pp.x);
		pp.y.ModMulK1(&_s);
		pp.y.ModSub(&_2Gn.y);
		startP = pp;
	}
	delete grp;
#if defined(_WIN64) && !defined(__CYGWIN__)
	WaitForSingleObject(bPload_mutex[threadid], INFINITE);
	tt->finished = 1;
	ReleaseMutex(bPload_mutex[threadid]);
#else	
	pthread_mutex_lock(&bPload_mutex[threadid]);
	tt->finished = 1;
	pthread_mutex_unlock(&bPload_mutex[threadid]);
	pthread_exit(NULL);
#endif
	return NULL;
}

void KECCAK_256(uint8_t *source, size_t size,uint8_t *dst)	{
	SHA3_256_CTX ctx;
	SHA3_256_Init(&ctx);
	SHA3_256_Update(&ctx,source,size);
	KECCAK_256_Final(dst,&ctx);
}

void generate_binaddress_eth(Point &publickey,unsigned char *dst_address)	{
	unsigned char bin_publickey[64];
	publickey.x.Get32Bytes(bin_publickey);
	publickey.y.Get32Bytes(bin_publickey+32);
	KECCAK_256(bin_publickey, 64, dst_address);	
}

#if defined(_WIN64) && !defined(__CYGWIN__)
DWORD WINAPI thread_process_bsgs_dance(LPVOID vargp) {
#else
void *thread_process_bsgs_dance(void *vargp)	{
#endif

	FILE *filekey;
	struct tothread *tt;
	char xpoint_raw[32],*aux_c,*hextemp;
	Int base_key,keyfound;
	Point base_point,point_aux,point_found;
	uint32_t i,j,k,l,r,salir,thread_number,flip_detector,entrar,cycles;
	
	IntGroup *grp = new IntGroup(CPU_GRP_SIZE / 2 + 1);
	Point startP;
	
	int hLength = (CPU_GRP_SIZE / 2 - 1);
	
	Int dx[CPU_GRP_SIZE / 2 + 1];
	Point pts[CPU_GRP_SIZE];

	Int dy;
	Int dyn;
	Int _s;
	Int _p;
	Int km,intaux;
	Point pp;
	Point pn;
	grp->Set(dx);

	
	tt = (struct tothread *)vargp;
	thread_number = tt->nt;
	free(tt);
	
	cycles = bsgs_aux / 1024;
	if(bsgs_aux % 1024 != 0)	{
		cycles++;
	}
	
	entrar = 1;
	
#if defined(_WIN64) && !defined(__CYGWIN__)
	WaitForSingleObject(bsgs_thread, INFINITE);
#else
	pthread_mutex_lock(&bsgs_thread);
#endif

	switch(rand() % 3)	{
		case 0:	//TOP
			base_key.Set(&n_range_end);
			base_key.Sub(&BSGS_N);
			n_range_end.Sub(&BSGS_N);
			if(base_key.IsLower(&BSGS_CURRENT))	{
				entrar = 0;
			}
			else	{
				n_range_end.Sub(&BSGS_N);
			}
		break;
		case 1: //BOTTOM
			base_key.Set(&BSGS_CURRENT);
			if(base_key.IsGreater(&n_range_end))	{
				entrar = 0;
			}
			else	{
				BSGS_CURRENT.Add(&BSGS_N);
			}
		break;
		case 2: //random - middle
			base_key.Rand(&BSGS_CURRENT,&n_range_end);
		break;
	}
#if defined(_WIN64) && !defined(__CYGWIN__)
	ReleaseMutex(bsgs_thread);
#else
	pthread_mutex_unlock(&bsgs_thread);
#endif

	

	intaux.Set(&BSGS_M);
	intaux.Mult(CPU_GRP_SIZE/2);
	
	flip_detector = FLIPBITLIMIT;
	
	
	/*
		while base_key is less than n_range_end then:
	*/
	while( entrar )	{
		if(thread_number == 0 && flip_detector == 0)	{
			memorycheck_bsgs();
			flip_detector = FLIPBITLIMIT;
		}
		
		if(FLAGMATRIX)	{
			aux_c = base_key.GetBase16();
			printf("[+] Thread 0x%s \n",aux_c);
			fflush(stdout);
			free(aux_c);
		}
		else	{
			if(FLAGQUIET == 0){
				aux_c = base_key.GetBase16();
				printf("\r[+] Thread 0x%s   \r",aux_c);
				fflush(stdout);
				free(aux_c);
				THREADOUTPUT = 1;
			}
		}
		
		base_point = secp->ComputePublicKey(&base_key);

		km.Set(&base_key);
		km.Neg();
		
		km.Add(&secp->order);
		km.Sub(&intaux);
		point_aux = secp->ComputePublicKey(&km);
		
		for(k = 0; k < bsgs_point_number ; k++)	{
			if(bsgs_found[k] == 0)	{
				if(base_point.equals(OriginalPointsBSGS[k]))	{
					hextemp = base_key.GetBase16();
					printf("[+] Thread Key found privkey %s  \n",hextemp);
					aux_c = secp->GetPublicKeyHex(OriginalPointsBSGScompressed[k],base_point);
					printf("[+] Publickey %s\n",aux_c);				
#if defined(_WIN64) && !defined(__CYGWIN__)
					WaitForSingleObject(write_keys, INFINITE);
#else
					pthread_mutex_lock(&write_keys);
#endif

					filekey = fopen("KEYFOUNDKEYFOUND.txt","a");
					if(filekey != NULL)	{
						fprintf(filekey,"Key found privkey %s\nPublickey %s\n",hextemp,aux_c);
						fclose(filekey);
					}
					free(hextemp);
					free(aux_c);
#if defined(_WIN64) && !defined(__CYGWIN__)
					ReleaseMutex(write_keys);
#else
					pthread_mutex_unlock(&write_keys);
#endif

					
					bsgs_found[k] = 1;
					salir = 1;
					for(l = 0; l < bsgs_point_number && salir; l++)	{
						salir &= bsgs_found[l];
					}
					if(salir)	{
						printf("All points were found\n");
						exit(0);
					}
				}
				else	{
					startP  = secp->AddDirect(OriginalPointsBSGS[k],point_aux);
					int j = 0;
					while( j < cycles && bsgs_found[k]== 0 )	{
					
						int i;
						
						for(i = 0; i < hLength; i++) {
							dx[i].ModSub(&GSn[i].x,&startP.x);
						}
						dx[i].ModSub(&GSn[i].x,&startP.x);  // For the first point
						dx[i+1].ModSub(&_2GSn.x,&startP.x); // For the next center point

						// Grouped ModInv
						grp->ModInv();
						
						/*
						We use the fact that P + i*G and P - i*G has the same deltax, so the same inverse
						We compute key in the positive and negative way from the center of the group
						*/

						// center point
						pts[CPU_GRP_SIZE / 2] = startP;
						
						for(i = 0; i<hLength; i++) {

							pp = startP;
							pn = startP;

							// P = startP + i*G
							dy.ModSub(&GSn[i].y,&pp.y);

							_s.ModMulK1(&dy,&dx[i]);        // s = (p2.y-p1.y)*inverse(p2.x-p1.x);
							_p.ModSquareK1(&_s);            // _p = pow2(s)

							pp.x.ModNeg();
							pp.x.ModAdd(&_p);
							pp.x.ModSub(&GSn[i].x);           // rx = pow2(s) - p1.x - p2.x;
							
#if 0
	  pp.y.ModSub(&GSn[i].x,&pp.x);
	  pp.y.ModMulK1(&_s);
	  pp.y.ModSub(&GSn[i].y);           // ry = - p2.y - s*(ret.x-p2.x);  
#endif

							// P = startP - i*G  , if (x,y) = i*G then (x,-y) = -i*G
							dyn.Set(&GSn[i].y);
							dyn.ModNeg();
							dyn.ModSub(&pn.y);

							_s.ModMulK1(&dyn,&dx[i]);       // s = (p2.y-p1.y)*inverse(p2.x-p1.x);
							_p.ModSquareK1(&_s);            // _p = pow2(s)

							pn.x.ModNeg();
							pn.x.ModAdd(&_p);
							pn.x.ModSub(&GSn[i].x);          // rx = pow2(s) - p1.x - p2.x;

#if 0
	  pn.y.ModSub(&GSn[i].x,&pn.x);
	  pn.y.ModMulK1(&_s);
	  pn.y.ModAdd(&GSn[i].y);          // ry = - p2.y - s*(ret.x-p2.x);  
#endif


							pts[CPU_GRP_SIZE / 2 + (i + 1)] = pp;
							pts[CPU_GRP_SIZE / 2 - (i + 1)] = pn;

						}

						// First point (startP - (GRP_SZIE/2)*G)
						pn = startP;
						dyn.Set(&GSn[i].y);
						dyn.ModNeg();
						dyn.ModSub(&pn.y);

						_s.ModMulK1(&dyn,&dx[i]);
						_p.ModSquareK1(&_s);

						pn.x.ModNeg();
						pn.x.ModAdd(&_p);
						pn.x.ModSub(&GSn[i].x);

#if 0
	pn.y.ModSub(&GSn[i].x,&pn.x);
	pn.y.ModMulK1(&_s);
	pn.y.ModAdd(&GSn[i].y);
#endif

						pts[0] = pn;
						
						for(int i = 0; i<CPU_GRP_SIZE && bsgs_found[k]== 0; i++) {
							pts[i].x.Get32Bytes((unsigned char*)xpoint_raw);
							r = bloom_check(&bloom_bP[((unsigned char)xpoint_raw[0])],xpoint_raw,32);
							if(r) {
								r = bsgs_secondcheck(&base_key,((j*1024) + i),k,&keyfound);
								if(r)	{
									hextemp = keyfound.GetBase16();
									printf("[+] Thread Key found privkey %s   \n",hextemp);
									point_found = secp->ComputePublicKey(&keyfound);
									aux_c = secp->GetPublicKeyHex(OriginalPointsBSGScompressed[k],point_found);
									printf("[+] Publickey %s\n",aux_c);
#if defined(_WIN64) && !defined(__CYGWIN__)
									WaitForSingleObject(write_keys, INFINITE);
#else
									pthread_mutex_lock(&write_keys);
#endif

									filekey = fopen("KEYFOUNDKEYFOUND.txt","a");
									if(filekey != NULL)	{
										fprintf(filekey,"Key found privkey %s\nPublickey %s\n",hextemp,aux_c);
										fclose(filekey);
									}
									free(hextemp);
									free(aux_c);
#if defined(_WIN64) && !defined(__CYGWIN__)
									ReleaseMutex(write_keys);
#else
									pthread_mutex_unlock(&write_keys);
#endif

									bsgs_found[k] = 1;
									salir = 1;
									for(l = 0; l < bsgs_point_number && salir; l++)	{
										salir &= bsgs_found[l];
									}
									if(salir)	{
										printf("All points were found\n");
										exit(0);
									}
								} //End if second check
							}//End if first check
							
						}// For for pts variable
						
						// Next start point (startP += (bsSize*GRP_SIZE).G)
						
						pp = startP;
						dy.ModSub(&_2GSn.y,&pp.y);

						_s.ModMulK1(&dy,&dx[i + 1]);
						_p.ModSquareK1(&_s);

						pp.x.ModNeg();
						pp.x.ModAdd(&_p);
						pp.x.ModSub(&_2GSn.x);

						pp.y.ModSub(&_2GSn.x,&pp.x);
						pp.y.ModMulK1(&_s);
						pp.y.ModSub(&_2GSn.y);
						startP = pp;
						
						j++;
					}//while all the aMP points
				}// end else
			}// End if 
		}
			
		steps[thread_number]++;
		flip_detector--;
		
#if defined(_WIN64) && !defined(__CYGWIN__)
		WaitForSingleObject(bsgs_thread, INFINITE);
#else
		pthread_mutex_lock(&bsgs_thread);
#endif

		switch(rand() % 3)	{
			case 0:	//TOP
				base_key.Set(&n_range_end);
				base_key.Sub(&BSGS_N);
				n_range_end.Sub(&BSGS_N);
				if(base_key.IsLower(&BSGS_CURRENT))	{
					entrar = 0;
				}
				else	{
					n_range_end.Sub(&BSGS_N);
				}
			break;
			case 1: //BOTTOM
				base_key.Set(&BSGS_CURRENT);
				if(base_key.IsGreater(&n_range_end))	{
					entrar = 0;
				}
				else	{
					BSGS_CURRENT.Add(&BSGS_N);
				}
			break;
			case 2: //random - middle
				base_key.Rand(&BSGS_CURRENT,&n_range_end);
			break;
		}
#if defined(_WIN64) && !defined(__CYGWIN__)
		ReleaseMutex(bsgs_thread);
#else
		pthread_mutex_unlock(&bsgs_thread);
#endif		
	}
	ends[thread_number] = 1;
	return NULL;
}

#if defined(_WIN64) && !defined(__CYGWIN__)
DWORD WINAPI thread_process_bsgs_backward(LPVOID vargp) {
#else
void *thread_process_bsgs_backward(void *vargp)	{
#endif
	FILE *filekey;
	struct tothread *tt;
	char xpoint_raw[32],*aux_c,*hextemp;
	Int base_key,keyfound;
	Point base_point,point_aux,point_found;
	uint32_t i,j,k,l,r,salir,thread_number,flip_detector,entrar,cycles;
	
	IntGroup *grp = new IntGroup(CPU_GRP_SIZE / 2 + 1);
	Point startP;
	
	int hLength = (CPU_GRP_SIZE / 2 - 1);
	
	Int dx[CPU_GRP_SIZE / 2 + 1];
	Point pts[CPU_GRP_SIZE];

	Int dy;
	Int dyn;
	Int _s;
	Int _p;
	Int km,intaux;
	Point pp;
	Point pn;
	grp->Set(dx);

	
	tt = (struct tothread *)vargp;
	thread_number = tt->nt;
	free(tt);

	cycles = bsgs_aux / 1024;
	if(bsgs_aux % 1024 != 0)	{
		cycles++;
	}

#if defined(_WIN64) && !defined(__CYGWIN__)
	WaitForSingleObject(bsgs_thread, INFINITE);
	n_range_end.Sub(&BSGS_N);
	base_key.Set(&n_range_end);
	ReleaseMutex(bsgs_thread);
#else
	pthread_mutex_lock(&bsgs_thread);
	n_range_end.Sub(&BSGS_N);
	base_key.Set(&n_range_end);
	pthread_mutex_unlock(&bsgs_thread);
#endif
	
	intaux.Set(&BSGS_M);
	intaux.Mult(CPU_GRP_SIZE/2);
	
	flip_detector = FLIPBITLIMIT;
	entrar = 1;
	
	/*
		while base_key is less than n_range_end then:
	*/
	while( entrar )	{
		if(thread_number == 0 && flip_detector == 0)	{
			memorycheck_bsgs();
			flip_detector = FLIPBITLIMIT;
		}
		
		if(FLAGMATRIX)	{
			aux_c = base_key.GetBase16();
			printf("[+] Thread 0x%s \n",aux_c);
			fflush(stdout);
			free(aux_c);
		}
		else	{
			if(FLAGQUIET == 0){
				aux_c = base_key.GetBase16();
				printf("\r[+] Thread 0x%s   \r",aux_c);
				fflush(stdout);
				free(aux_c);
				THREADOUTPUT = 1;
			}
		}
		
		base_point = secp->ComputePublicKey(&base_key);

		km.Set(&base_key);
		km.Neg();
		
		km.Add(&secp->order);
		km.Sub(&intaux);
		point_aux = secp->ComputePublicKey(&km);
		
		for(k = 0; k < bsgs_point_number ; k++)	{
			if(bsgs_found[k] == 0)	{
				if(base_point.equals(OriginalPointsBSGS[k]))	{
					hextemp = base_key.GetBase16();
					printf("[+] Thread Key found privkey %s  \n",hextemp);
					aux_c = secp->GetPublicKeyHex(OriginalPointsBSGScompressed[k],base_point);
					printf("[+] Publickey %s\n",aux_c);
					
#if defined(_WIN64) && !defined(__CYGWIN__)
					WaitForSingleObject(write_keys, INFINITE);
#else
					pthread_mutex_lock(&write_keys);
#endif

					filekey = fopen("KEYFOUNDKEYFOUND.txt","a");
					if(filekey != NULL)	{
						fprintf(filekey,"Key found privkey %s\nPublickey %s\n",hextemp,aux_c);
						fclose(filekey);
					}
					free(hextemp);
					free(aux_c);
#if defined(_WIN64) && !defined(__CYGWIN__)
					ReleaseMutex(write_keys);
#else
					pthread_mutex_unlock(&write_keys);
#endif					
					bsgs_found[k] = 1;
					salir = 1;
					for(l = 0; l < bsgs_point_number && salir; l++)	{
						salir &= bsgs_found[l];
					}
					if(salir)	{
						printf("All points were found\n");
						exit(0);
					}
				}
				else	{
					startP  = secp->AddDirect(OriginalPointsBSGS[k],point_aux);
					int j = 0;
					while( j < cycles && bsgs_found[k]== 0 )	{
					
						int i;
						
						for(i = 0; i < hLength; i++) {
							dx[i].ModSub(&GSn[i].x,&startP.x);
						}
						dx[i].ModSub(&GSn[i].x,&startP.x);  // For the first point
						dx[i+1].ModSub(&_2GSn.x,&startP.x); // For the next center point

						// Grouped ModInv
						grp->ModInv();
						
						/*
						We use the fact that P + i*G and P - i*G has the same deltax, so the same inverse
						We compute key in the positive and negative way from the center of the group
						*/

						// center point
						pts[CPU_GRP_SIZE / 2] = startP;
						
						for(i = 0; i<hLength; i++) {

							pp = startP;
							pn = startP;

							// P = startP + i*G
							dy.ModSub(&GSn[i].y,&pp.y);

							_s.ModMulK1(&dy,&dx[i]);        // s = (p2.y-p1.y)*inverse(p2.x-p1.x);
							_p.ModSquareK1(&_s);            // _p = pow2(s)

							pp.x.ModNeg();
							pp.x.ModAdd(&_p);
							pp.x.ModSub(&GSn[i].x);           // rx = pow2(s) - p1.x - p2.x;
							
#if 0
	  pp.y.ModSub(&GSn[i].x,&pp.x);
	  pp.y.ModMulK1(&_s);
	  pp.y.ModSub(&GSn[i].y);           // ry = - p2.y - s*(ret.x-p2.x);  
#endif

							// P = startP - i*G  , if (x,y) = i*G then (x,-y) = -i*G
							dyn.Set(&GSn[i].y);
							dyn.ModNeg();
							dyn.ModSub(&pn.y);

							_s.ModMulK1(&dyn,&dx[i]);       // s = (p2.y-p1.y)*inverse(p2.x-p1.x);
							_p.ModSquareK1(&_s);            // _p = pow2(s)

							pn.x.ModNeg();
							pn.x.ModAdd(&_p);
							pn.x.ModSub(&GSn[i].x);          // rx = pow2(s) - p1.x - p2.x;

#if 0
	  pn.y.ModSub(&GSn[i].x,&pn.x);
	  pn.y.ModMulK1(&_s);
	  pn.y.ModAdd(&GSn[i].y);          // ry = - p2.y - s*(ret.x-p2.x);  
#endif


							pts[CPU_GRP_SIZE / 2 + (i + 1)] = pp;
							pts[CPU_GRP_SIZE / 2 - (i + 1)] = pn;

						}

						// First point (startP - (GRP_SZIE/2)*G)
						pn = startP;
						dyn.Set(&GSn[i].y);
						dyn.ModNeg();
						dyn.ModSub(&pn.y);

						_s.ModMulK1(&dyn,&dx[i]);
						_p.ModSquareK1(&_s);

						pn.x.ModNeg();
						pn.x.ModAdd(&_p);
						pn.x.ModSub(&GSn[i].x);

#if 0
	pn.y.ModSub(&GSn[i].x,&pn.x);
	pn.y.ModMulK1(&_s);
	pn.y.ModAdd(&GSn[i].y);
#endif

						pts[0] = pn;
						
						for(int i = 0; i<CPU_GRP_SIZE && bsgs_found[k]== 0; i++) {
							pts[i].x.Get32Bytes((unsigned char*)xpoint_raw);
							r = bloom_check(&bloom_bP[((unsigned char)xpoint_raw[0])],xpoint_raw,32);
							if(r) {
								r = bsgs_secondcheck(&base_key,((j*1024) + i),k,&keyfound);
								if(r)	{
									hextemp = keyfound.GetBase16();
									printf("[+] Thread Key found privkey %s   \n",hextemp);
									point_found = secp->ComputePublicKey(&keyfound);
									aux_c = secp->GetPublicKeyHex(OriginalPointsBSGScompressed[k],point_found);
									printf("[+] Publickey %s\n",aux_c);
#if defined(_WIN64) && !defined(__CYGWIN__)
									WaitForSingleObject(write_keys, INFINITE);
#else
									pthread_mutex_lock(&write_keys);
#endif

									filekey = fopen("KEYFOUNDKEYFOUND.txt","a");
									if(filekey != NULL)	{
										fprintf(filekey,"Key found privkey %s\nPublickey %s\n",hextemp,aux_c);
										fclose(filekey);
									}
									free(hextemp);
									free(aux_c);
#if defined(_WIN64) && !defined(__CYGWIN__)
									ReleaseMutex(write_keys);
#else
									pthread_mutex_unlock(&write_keys);
#endif

									bsgs_found[k] = 1;
									salir = 1;
									for(l = 0; l < bsgs_point_number && salir; l++)	{
										salir &= bsgs_found[l];
									}
									if(salir)	{
										printf("All points were found\n");
										exit(0);
									}
								} //End if second check
							}//End if first check
							
						}// For for pts variable
						
						// Next start point (startP += (bsSize*GRP_SIZE).G)
						
						pp = startP;
						dy.ModSub(&_2GSn.y,&pp.y);

						_s.ModMulK1(&dy,&dx[i + 1]);
						_p.ModSquareK1(&_s);

						pp.x.ModNeg();
						pp.x.ModAdd(&_p);
						pp.x.ModSub(&_2GSn.x);

						pp.y.ModSub(&_2GSn.x,&pp.x);
						pp.y.ModMulK1(&_s);
						pp.y.ModSub(&_2GSn.y);
						startP = pp;
						
						j++;
					}//while all the aMP points
				}// end else
			}// End if 
		}
			
		steps[thread_number]++;
		flip_detector--;
		
#if defined(_WIN64) && !defined(__CYGWIN__)
		WaitForSingleObject(bsgs_thread, INFINITE);
#else
		pthread_mutex_lock(&bsgs_thread);
#endif

		n_range_end.Sub(&BSGS_N);
		if(n_range_end.IsLower(&n_range_start))	{
			entrar = 0;
		}
		else	{
			base_key.Set(&n_range_end);
		}
#if defined(_WIN64) && !defined(__CYGWIN__)
		ReleaseMutex(bsgs_thread);
#else
		pthread_mutex_unlock(&bsgs_thread);
#endif

	}
	ends[thread_number] = 1;
	return NULL;
}


#if defined(_WIN64) && !defined(__CYGWIN__)
DWORD WINAPI thread_process_bsgs_both(LPVOID vargp) {
#else
void *thread_process_bsgs_both(void *vargp)	{
#endif
	FILE *filekey;
	struct tothread *tt;
	char xpoint_raw[32],*aux_c,*hextemp;
	Int base_key,keyfound;
	Point base_point,point_aux,point_found;
	uint32_t i,j,k,l,r,salir,thread_number,flip_detector,entrar,cycles;
	
	IntGroup *grp = new IntGroup(CPU_GRP_SIZE / 2 + 1);
	Point startP;
	
	int hLength = (CPU_GRP_SIZE / 2 - 1);
	
	Int dx[CPU_GRP_SIZE / 2 + 1];
	Point pts[CPU_GRP_SIZE];

	Int dy;
	Int dyn;
	Int _s;
	Int _p;
	Int km,intaux;
	Point pp;
	Point pn;
	grp->Set(dx);

	
	tt = (struct tothread *)vargp;
	thread_number = tt->nt;
	free(tt);
	
	cycles = bsgs_aux / 1024;
	if(bsgs_aux % 1024 != 0)	{
		cycles++;
	}
	
	entrar = 1;
	
#if defined(_WIN64) && !defined(__CYGWIN__)
	WaitForSingleObject(bsgs_thread, INFINITE);
#else
	pthread_mutex_lock(&bsgs_thread);
#endif

	r = rand() % 2;
	//if(FLAGDEBUG) printf("[D] was %s\n",r ? "Bottom":"TOP");
	switch(r)	{
		case 0:	//TOP
			base_key.Set(&n_range_end);
			base_key.Sub(&BSGS_N);
			if(base_key.IsLowerOrEqual(&BSGS_CURRENT))	{
				entrar = 0;
			}
			else	{
				n_range_end.Sub(&BSGS_N);
			}
		break;
		case 1: //BOTTOM
			base_key.Set(&BSGS_CURRENT);
			if(base_key.IsGreaterOrEqual(&n_range_end))	{
				entrar = 0;
			}
			else	{
				BSGS_CURRENT.Add(&BSGS_N);
			}
		break;
	}
#if defined(_WIN64) && !defined(__CYGWIN__)
	ReleaseMutex(bsgs_thread);
#else
	pthread_mutex_unlock(&bsgs_thread);
#endif

	
	intaux.Set(&BSGS_M);
	intaux.Mult(CPU_GRP_SIZE/2);
	
	flip_detector = FLIPBITLIMIT;
	
	
	/*
		while BSGS_CURRENT is less than n_range_end 
	*/
	while( entrar )	{
		
		if(thread_number == 0 && flip_detector == 0)	{
			memorycheck_bsgs();
			flip_detector = FLIPBITLIMIT;
		}
		if(FLAGMATRIX)	{
			aux_c = base_key.GetBase16();
			printf("[+] Thread 0x%s \n",aux_c);
			fflush(stdout);
			free(aux_c);
		}
		else	{
			if(FLAGQUIET == 0){
				aux_c = base_key.GetBase16();
				printf("\r[+] Thread 0x%s   \r",aux_c);
				fflush(stdout);
				free(aux_c);
				THREADOUTPUT = 1;
			}
		}
		
		base_point = secp->ComputePublicKey(&base_key);

		km.Set(&base_key);
		km.Neg();
		
		km.Add(&secp->order);
		km.Sub(&intaux);
		point_aux = secp->ComputePublicKey(&km);
		
		for(k = 0; k < bsgs_point_number ; k++)	{
			if(bsgs_found[k] == 0)	{
				if(base_point.equals(OriginalPointsBSGS[k]))	{
					hextemp = base_key.GetBase16();
					printf("[+] Thread Key found privkey %s  \n",hextemp);
					aux_c = secp->GetPublicKeyHex(OriginalPointsBSGScompressed[k],base_point);
					printf("[+] Publickey %s\n",aux_c);
#if defined(_WIN64) && !defined(__CYGWIN__)
					WaitForSingleObject(write_keys, INFINITE);
#else
					pthread_mutex_lock(&write_keys);
#endif
					filekey = fopen("KEYFOUNDKEYFOUND.txt","a");
					if(filekey != NULL)	{
						fprintf(filekey,"Key found privkey %s\nPublickey %s\n",hextemp,aux_c);
						fclose(filekey);
					}
					free(hextemp);
					free(aux_c);
#if defined(_WIN64) && !defined(__CYGWIN__)
					ReleaseMutex(write_keys);
#else
					pthread_mutex_unlock(&write_keys);
#endif
					
					bsgs_found[k] = 1;
					salir = 1;
					for(j = 0; l < bsgs_point_number && salir; l++)	{
						salir &= bsgs_found[l];
					}
					if(salir)	{
						printf("All points were found\n");
						exit(0);
					}
				}
				else	{
					startP  = secp->AddDirect(OriginalPointsBSGS[k],point_aux);
					int j = 0;
					while( j < cycles && bsgs_found[k]== 0 )	{
					
						int i;
						
						for(i = 0; i < hLength; i++) {
							dx[i].ModSub(&GSn[i].x,&startP.x);
						}
						dx[i].ModSub(&GSn[i].x,&startP.x);  // For the first point
						dx[i+1].ModSub(&_2GSn.x,&startP.x); // For the next center point

						// Grouped ModInv
						grp->ModInv();
						
						/*
						We use the fact that P + i*G and P - i*G has the same deltax, so the same inverse
						We compute key in the positive and negative way from the center of the group
						*/

						// center point
						pts[CPU_GRP_SIZE / 2] = startP;
						
						for(i = 0; i<hLength; i++) {

							pp = startP;
							pn = startP;

							// P = startP + i*G
							dy.ModSub(&GSn[i].y,&pp.y);

							_s.ModMulK1(&dy,&dx[i]);        // s = (p2.y-p1.y)*inverse(p2.x-p1.x);
							_p.ModSquareK1(&_s);            // _p = pow2(s)

							pp.x.ModNeg();
							pp.x.ModAdd(&_p);
							pp.x.ModSub(&GSn[i].x);           // rx = pow2(s) - p1.x - p2.x;
							
#if 0
	  pp.y.ModSub(&GSn[i].x,&pp.x);
	  pp.y.ModMulK1(&_s);
	  pp.y.ModSub(&GSn[i].y);           // ry = - p2.y - s*(ret.x-p2.x);  
#endif

							// P = startP - i*G  , if (x,y) = i*G then (x,-y) = -i*G
							dyn.Set(&GSn[i].y);
							dyn.ModNeg();
							dyn.ModSub(&pn.y);

							_s.ModMulK1(&dyn,&dx[i]);       // s = (p2.y-p1.y)*inverse(p2.x-p1.x);
							_p.ModSquareK1(&_s);            // _p = pow2(s)

							pn.x.ModNeg();
							pn.x.ModAdd(&_p);
							pn.x.ModSub(&GSn[i].x);          // rx = pow2(s) - p1.x - p2.x;

#if 0
	  pn.y.ModSub(&GSn[i].x,&pn.x);
	  pn.y.ModMulK1(&_s);
	  pn.y.ModAdd(&GSn[i].y);          // ry = - p2.y - s*(ret.x-p2.x);  
#endif


							pts[CPU_GRP_SIZE / 2 + (i + 1)] = pp;
							pts[CPU_GRP_SIZE / 2 - (i + 1)] = pn;

						}

						// First point (startP - (GRP_SZIE/2)*G)
						pn = startP;
						dyn.Set(&GSn[i].y);
						dyn.ModNeg();
						dyn.ModSub(&pn.y);

						_s.ModMulK1(&dyn,&dx[i]);
						_p.ModSquareK1(&_s);

						pn.x.ModNeg();
						pn.x.ModAdd(&_p);
						pn.x.ModSub(&GSn[i].x);

#if 0
	pn.y.ModSub(&GSn[i].x,&pn.x);
	pn.y.ModMulK1(&_s);
	pn.y.ModAdd(&GSn[i].y);
#endif

						pts[0] = pn;
						
						for(int i = 0; i<CPU_GRP_SIZE && bsgs_found[k]== 0; i++) {
							pts[i].x.Get32Bytes((unsigned char*)xpoint_raw);
							r = bloom_check(&bloom_bP[((unsigned char)xpoint_raw[0])],xpoint_raw,32);
							if(r) {
								r = bsgs_secondcheck(&base_key,((j*1024) + i),k,&keyfound);
								if(r)	{
									hextemp = keyfound.GetBase16();
									printf("[+] Thread Key found privkey %s   \n",hextemp);
									point_found = secp->ComputePublicKey(&keyfound);
									aux_c = secp->GetPublicKeyHex(OriginalPointsBSGScompressed[k],point_found);
									printf("[+] Publickey %s\n",aux_c);
#if defined(_WIN64) && !defined(__CYGWIN__)
									WaitForSingleObject(write_keys, INFINITE);
#else
									pthread_mutex_lock(&write_keys);
#endif

									filekey = fopen("KEYFOUNDKEYFOUND.txt","a");
									if(filekey != NULL)	{
										fprintf(filekey,"Key found privkey %s\nPublickey %s\n",hextemp,aux_c);
										fclose(filekey);
									}
									free(hextemp);
									free(aux_c);
#if defined(_WIN64) && !defined(__CYGWIN__)
									ReleaseMutex(write_keys);
#else
									pthread_mutex_unlock(&write_keys);
#endif

									bsgs_found[k] = 1;
									salir = 1;
									for(l = 0; l < bsgs_point_number && salir; l++)	{
										salir &= bsgs_found[l];
									}
									if(salir)	{
										printf("All points were found\n");
										exit(0);
									}
								} //End if second check
							}//End if first check
							
						}// For for pts variable
						
						// Next start point (startP += (bsSize*GRP_SIZE).G)
						
						pp = startP;
						dy.ModSub(&_2GSn.y,&pp.y);

						_s.ModMulK1(&dy,&dx[i + 1]);
						_p.ModSquareK1(&_s);

						pp.x.ModNeg();
						pp.x.ModAdd(&_p);
						pp.x.ModSub(&_2GSn.x);

						pp.y.ModSub(&_2GSn.x,&pp.x);
						pp.y.ModMulK1(&_s);
						pp.y.ModSub(&_2GSn.y);
						startP = pp;
						
						j++;
					}//while all the aMP points
				}// end else
			}// End if 
		}
			
		steps[thread_number]++;
		flip_detector--;
		
#if defined(_WIN64) && !defined(__CYGWIN__)
		WaitForSingleObject(bsgs_thread, INFINITE);
#else
		pthread_mutex_lock(&bsgs_thread);
#endif

		switch(rand() % 2)	{
			case 0:	//TOP
				base_key.Set(&n_range_end);
				base_key.Sub(&BSGS_N);
				if(base_key.IsLowerOrEqual(&BSGS_CURRENT))	{
					entrar = 0;
				}
				else	{
					n_range_end.Sub(&BSGS_N);
				}
			break;
			case 1: //BOTTOM
				base_key.Set(&BSGS_CURRENT);
				if(base_key.IsGreaterOrEqual(&n_range_end))	{
					entrar = 0;
				}
				else	{
					BSGS_CURRENT.Add(&BSGS_N);
				}
			break;
		}
#if defined(_WIN64) && !defined(__CYGWIN__)
		ReleaseMutex(bsgs_thread);
#else
		pthread_mutex_unlock(&bsgs_thread);
#endif		
	}
	ends[thread_number] = 1;
	return NULL;
}

void memorycheck_bsgs()	{
	char current_checksum[32];
	char *hextemp,*aux_c;
	//if(FLAGDEBUG )printf("[D] Performing Memory checksum  \n");
	sha256((uint8_t*)bPtable,bytes,(uint8_t*)current_checksum);
	if(memcmp(current_checksum,checksum,32) != 0 || memcmp(current_checksum,checksum_backup,32) != 0)	{
		fprintf(stderr,"[E] Memory checksum mismatch, this should not happen but actually happened\nA bit in the memory was flipped by : electrical malfuntion, radiation or a cosmic ray\n");
		hextemp = tohex(current_checksum,32);
		aux_c = tohex(checksum,32);
		fprintf(stderr,"Current Checksum: %s\n",hextemp);
		fprintf(stderr,"Saved Checksum: %s\n",aux_c);
		aux_c = tohex(checksum_backup,32);
		fprintf(stderr,"Backup Checksum: %s\nExit!\n",aux_c);
		exit(0);
	}
}


void set_minikey(char *buffer,char *rawbuffer,int length)	{
	for(int i = 0;  i < length; i++)	{
		
		buffer[i] = Ccoinbuffer[(uint8_t)rawbuffer[i]];
	}
}

bool increment_minikey_index(char *buffer,char *rawbuffer,int index)	{
	if(rawbuffer[index] < 57){
		rawbuffer[index]++;
		buffer[index] = Ccoinbuffer[rawbuffer[index]];
	}
	else	{
		rawbuffer[index] = 0x00;
		buffer[index] = Ccoinbuffer[0];
		if(index>0)	{
			return increment_minikey_index(buffer,rawbuffer,index-1);
		}
		else	{
			return false;
		}
	}
	return true;
}

void increment_minikey_N(char *rawbuffer)	{
	int i = 20,j = 0;
	while( i > 0 && j < minikey_n_limit)	{
		rawbuffer[i] = rawbuffer[i] + minikeyN[i];
		if(rawbuffer[i] > 57)	{
			rawbuffer[i] = rawbuffer[i] % 58;
			rawbuffer[i-1]++;
		}
		i--;
		j++;
	}
}


#define BUFFMINIKEY(buff,src) \
(buff)[ 0] = (uint32_t)src[ 0] << 24 | (uint32_t)src[ 1] << 16 | (uint32_t)src[ 2] << 8 | (uint32_t)src[ 3]; \
(buff)[ 1] = (uint32_t)src[ 4] << 24 | (uint32_t)src[ 5] << 16 | (uint32_t)src[ 6] << 8 | (uint32_t)src[ 7]; \
(buff)[ 2] = (uint32_t)src[ 8] << 24 | (uint32_t)src[ 9] << 16 | (uint32_t)src[10] << 8 | (uint32_t)src[11]; \
(buff)[ 3] = (uint32_t)src[12] << 24 | (uint32_t)src[13] << 16 | (uint32_t)src[14] << 8 | (uint32_t)src[15]; \
(buff)[ 4] = (uint32_t)src[16] << 24 | (uint32_t)src[17] << 16 | (uint32_t)src[18] << 8 | (uint32_t)src[19]; \
(buff)[ 5] = (uint32_t)src[20] << 24 | (uint32_t)src[21] << 16 | 0x8000; \
(buff)[ 6] = 0; \
(buff)[ 7] = 0; \
(buff)[ 8] = 0; \
(buff)[ 9] = 0; \
(buff)[10] = 0; \
(buff)[11] = 0; \
(buff)[12] = 0; \
(buff)[13] = 0; \
(buff)[14] = 0; \
(buff)[15] = 0xB0;	//176 bits => 22 BYTES


void sha256sse_22(uint8_t *src0, uint8_t *src1, uint8_t *src2, uint8_t *src3, uint8_t *dst0, uint8_t *dst1, uint8_t *dst2, uint8_t *dst3)	{
  uint32_t b0[16];
  uint32_t b1[16];
  uint32_t b2[16];
  uint32_t b3[16];
  BUFFMINIKEY(b0, src0);
  BUFFMINIKEY(b1, src1);
  BUFFMINIKEY(b2, src2);
  BUFFMINIKEY(b3, src3);
  sha256sse_1B(b0, b1, b2, b3, dst0, dst1, dst2, dst3);
}


#define BUFFMINIKEYCHECK(buff,src) \
(buff)[ 0] = (uint32_t)src[ 0] << 24 | (uint32_t)src[ 1] << 16 | (uint32_t)src[ 2] << 8 | (uint32_t)src[ 3]; \
(buff)[ 1] = (uint32_t)src[ 4] << 24 | (uint32_t)src[ 5] << 16 | (uint32_t)src[ 6] << 8 | (uint32_t)src[ 7]; \
(buff)[ 2] = (uint32_t)src[ 8] << 24 | (uint32_t)src[ 9] << 16 | (uint32_t)src[10] << 8 | (uint32_t)src[11]; \
(buff)[ 3] = (uint32_t)src[12] << 24 | (uint32_t)src[13] << 16 | (uint32_t)src[14] << 8 | (uint32_t)src[15]; \
(buff)[ 4] = (uint32_t)src[16] << 24 | (uint32_t)src[17] << 16 | (uint32_t)src[18] << 8 | (uint32_t)src[19]; \
(buff)[ 5] = (uint32_t)src[20] << 24 | (uint32_t)src[21] << 16 | (uint32_t)src[22] << 8 | 0x80; \
(buff)[ 6] = 0; \
(buff)[ 7] = 0; \
(buff)[ 8] = 0; \
(buff)[ 9] = 0; \
(buff)[10] = 0; \
(buff)[11] = 0; \
(buff)[12] = 0; \
(buff)[13] = 0; \
(buff)[14] = 0; \
(buff)[15] = 0xB8;	//184 bits => 23 BYTES

void sha256sse_23(uint8_t *src0, uint8_t *src1, uint8_t *src2, uint8_t *src3, uint8_t *dst0, uint8_t *dst1, uint8_t *dst2, uint8_t *dst3)	{
  uint32_t b0[16];
  uint32_t b1[16];
  uint32_t b2[16];
  uint32_t b3[16];
  BUFFMINIKEYCHECK(b0, src0);
  BUFFMINIKEYCHECK(b1, src1);
  BUFFMINIKEYCHECK(b2, src2);
  BUFFMINIKEYCHECK(b3, src3);
  sha256sse_1B(b0, b1, b2, b3, dst0, dst1, dst2, dst3);
}


/*
#if defined(_WIN64) && !defined(__CYGWIN__)
DWORD WINAPI thread_process_check_file_btc(LPVOID vargp) {
#else
void *thread_process_check_file_btc(void *vargp)	{
#endif
	FILE *input;
	FILE *keys;
	Point publickey;
	Int key_mpz;
	struct tothread *tt;
	uint64_t count;
	char publickeyhashrmd160_uncompress[4][20];
	char public_key_compressed_hex[67],public_key_uncompressed_hex[131];
	char hexstrpoint[65],rawvalue[4][32];
	char address[4][40],buffer[1024],digest[32],buffer_b58[21];
	char *hextemp,*rawbuffer;
	int r,thread_number,continue_flag = 1,k,j,count_valid;
	Int counter;
	tt = (struct tothread *)vargp;
	thread_number = tt->nt;
	free(tt);
	count = 0;
	input = fopen("test.txt","r");
	if(input != NULL)	{
		while(!feof(input))	{
			memset(buffer,0,1024);
			fgets(buffer,1024,input);
			trim(buffer,"\t\n\r ");
			if(strlen(buffer) > 0)	{
				printf("Checking %s\n",buffer);
				key_mpz.SetBase16(buffer);
				publickey = secp->ComputePublicKey(&key_mpz);
				secp->GetHash160(P2PKH,true,publickey,(uint8_t*)publickeyhashrmd160_uncompress[0]);
				secp->GetHash160(P2PKH,false,publickey,(uint8_t*)publickeyhashrmd160_uncompress[1]);
				for(k = 0; k < 2 ; k++)	{
					r = bloom_check(&bloom,publickeyhashrmd160_uncompress[k],20);
					if(r) {
						r = searchbinary(addressTable,publickeyhashrmd160_uncompress[k],N);
						if(r) {
							hextemp = key_mpz.GetBase16();
							secp->GetPublicKeyHex(false,publickey,public_key_uncompressed_hex);
#if defined(_WIN64) && !defined(__CYGWIN__)
							WaitForSingleObject(write_keys, INFINITE);
#else
							pthread_mutex_lock(&write_keys);
#endif
					
							keys = fopen("KEYFOUNDKEYFOUND.txt","a+");
							rmd160toaddress_dst(publickeyhashrmd160_uncompress[k],address[k]);
							if(keys != NULL)	{
								fprintf(keys,"Private Key: %s\npubkey: %s\n",hextemp,public_key_uncompressed_hex);
								fclose(keys);
							}
							printf("\nHIT!! Private Key: %s\npubkey: %s\n",hextemp,public_key_uncompressed_hex);
#if defined(_WIN64) && !defined(__CYGWIN__)
							ReleaseMutex(write_keys);
#else
							pthread_mutex_unlock(&write_keys);
#endif
							free(hextemp);

						}
					}
				}
				count++;
			}
		}
		printf("Totale keys readed %i\n",count);
	}
	return NULL;
}
*/