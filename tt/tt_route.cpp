/* <DESC>
 * simple HTTP POST using the easy interface
 * </DESC>
 */ 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <unistd.h>
extern "C"
{
#include <curl/curl.h>
#include <openssl/md5.h>
}
#include <string>
#include <set>

#include "version.h"
#include "../h/defs.h"

#undef malloc
#undef free
#undef realloc

extern "C"
{
extern Id global_lock();
extern Id global_unlock();

char * calc_md5(char buf[MD5_DIGEST_LENGTH*2], const char * input);
}								

const int UI_PKT = 1;
const int UI_AGG = 2;

#define LUI 38+5 /* say */
#define MAX_ITEMS 1000

typedef struct Tt_ui_s
{ char wh;
  int price;
	char c[LUI+1];
} Tt_ui_t, *Tt_ui;

Tt_ui_t g_tt_uis[MAX_ITEMS];
int g_tt_num_ui;

Char dbg_file_name[] = "tt.log";  // to resolve symbol

Char g_dbg_file_name[] = "tt.log";
FILE * g_dbg_file;

Char g_err_log_name[] = "tt.log";
FILE * g_err_log;

const Char nulls[1024] = { (char)0 };

static int a_opt,b_opt,d_opt,e_opt,k_opt,l_opt,n_opt,p_opt,r_opt,t_opt,v_opt;
static int g_clamp = 1000000;

static int g_longest_ui = 50;

static const char * g_authurl[3] = 
		{ "https://prod.auth.eu.tobaccotracing.com",
			"https://auth-uat.ukgateway.delarue.com/auth/realms/gateway-uk/protocol/openid-connect",
			"https://auth-ukgateway.delarue.com/auth/realms/gateway-uk/protocol/openid-connect",
		};
//static const char * g_authurl = "https://qa.auth.eu.tobaccotracing.com";
//static const char * g_authurl = "https://onboarding.auth.eu.tobaccotracing.com";
static const char * g_routerurl[3] = 
		{	"https://prod.router.eu.tobaccotracing.com/v1",
			"https://uat.ukgateway.delarue.com/v1/gateway/event",
			"https://ukgateway.delarue.com/v1/gateway/event",
		};
//static const char * g_routerurl = "https://qa.router.eu.tobaccotracing.com";
//static const char* g_routerurl ="https://onboarding.router.eu.tobaccotracing.com";
	
const char * g_post_m1 = "Postman-Token: ea87f94c-9296-4772-9c8c-8a9d2d6b5b3c";
const char * g_post_m2 = "Postman-Token: 3c726cbc-a9d6-4c28-bf33-53c461bbcef5";

//static const char * g_client_id = "abcdefghijklm";
//static const char * g_client_id = "3ojkiua8t230neqhi1tcnvtuv4"; QA env

static const char * g_client_id[3] = 
		{ "6dulrsbs79o0nhegdc24c7r9q5",
			"QCGDLREbTTKd7yb1tYi",
			"QCGDLREbTTKd7WlTDIg"
		};
static const char * g_client_secret[3] = 
		{	"l3rpmo1p94tne8dss6bsglai8ol7scmclvvv4unons8elu3gq1b",
			"9e85bbb6-527c-4c87-86af-522147181202",
			"0203936e-8811-4a3c-b23c-99d140d13834",
		};

static char g_gw_cookie[200];

static			 char 	g_our_eoid[100] = "QCGDLREbTTKd7";
static const char * g_eo_pfx = "QCGDLRE";
static const char * g_our_fac_id = "QCGDLRFirM4zX";

static const char * g_eoids[3] = {"AKKPEZENIR9IT2GDXU9F", // EO_
                                  "ZSZSLXRVYFEZQCX692RP",
                                  "R78DNU39CZ9BKERK4A4M"};
                                  
static const char * g_fids[3] = {"AKKPEZENIR9IT2GDXU9F",
                                 "9E72C92N79Z9QRY73U9V", // FID_
                                 "AKKPEZENIR9IT2GDXU9F"};
static int g_eoid_ix;

static char * g_check_eoid = NULL;

static CURL * g_curl;
static char g_access_token[4096];
static char * g_dd;

static char * g_rec;

const int SAFETYBUFFER = 100000;

#if 0
typedef enum {
  CURLINFO_TEXT = 0,
  CURLINFO_HEADER_IN,    /* 1 */
  CURLINFO_HEADER_OUT,   /* 2 */
  CURLINFO_DATA_IN,      /* 3 */
  CURLINFO_DATA_OUT,     /* 4 */
  CURLINFO_SSL_DATA_IN,  /* 5 */
  CURLINFO_SSL_DATA_OUT, /* 6 */
  CURLINFO_END
} curl_infotype;
#endif

const char * dtype[] = { "Text", "Headin", "Heado", "In", "Out", "SSLIn", "SSLOut", "End",};

int debug_callback(CURL *handle,
                   curl_infotype type = CURLINFO_TEXT,
                   char *data = NULL,
                   size_t size = 0,
                   void *userptr = NULL)
{ static FILE * err_out;

	if (handle == NULL)
	{ if (err_out != NULL)
			fclose(err_out);
	}	
	else
	{	if (err_out == NULL)
			err_out = fopen("tt_diags","w");

		if (err_out != NULL)
	 	{ fprintf(err_out, "Type: %s (%d) %132s\n", dtype[type], size, data);
	  }
	}
}
 
class Illegal_Eids
{
  public:
    void load_eids();
    
    bool got_eid(std::string id);
  std::set<std::string> c;
};

void Illegal_Eids::load_eids()

{ char row_buf[133];
  char * ln;
  FILE * ip = fopen("tt_err_log", "r");
  if (ip != NULL)
    while ((ln = fgets(row_buf, sizeof(row_buf) -1, ip)) != NULL)
    {
      if (strstr(ln, "not recognised") == NULL ||
          strstr(ln, "EOID") != ln)
        continue;
      char * t = ln+4;
      while (*++t != ' ' && *t != 0)
        ;
      *t = 0;
      c.insert(std::string(ln+5));
    }
}

bool Illegal_Eids::got_eid(std::string id)

{ return c.find(id) != c.end();
}

Illegal_Eids ill_eids;

size_t g_data_size;
size_t curlWriteFunction(void* ptr, size_t size/*always==1*/,
												 size_t nmemb, void* userdata)
{
	char ** s =(char**)userdata;
	const char* input = (const char*)ptr;
	if (nmemb==0)
		return 0;
	if(!*s)
		*s = (char*)malloc(nmemb+2+SAFETYBUFFER);
	else
		*s = (char*)realloc(*s, g_data_size+nmemb+2+SAFETYBUFFER);
	memcpy(*s+g_data_size, input, nmemb);
	g_data_size += nmemb;
	(*s)[g_data_size]='\n';
	(*s)[++g_data_size]=0;
	return nmemb;
}

const char * do_request_access()

{ char * authurl_env = getenv("TT_ROUTER");
	if (authurl_env != NULL)
		g_authurl[d_opt]  = authurl_env;

	char * our_eoid = getenv("TT_EOID");
	if (our_eoid != NULL)
		strpcpy(g_our_eoid, our_eoid, sizeof(g_our_eoid) - 1);
                   					        // In windows, this inits the winsock stuff
	curl_global_init(CURL_GLOBAL_ALL);
							 
	char cbuff[512];
	CURL *curl = curl_easy_init();
  g_curl = curl;

	CURLcode cc = curl_easy_setopt(curl,CURLOPT_CUSTOMREQUEST,/*d_opt?"GET":*/"POST");
	if (cc != OK)
		return "Setopt cr failed";
									//		 "http://{{authURL}}/token?grant_type=client_credentials"

	sprintf(cbuff, d_opt ? "%s/token" :
											 "%s/token?grant_type=client_credentials", g_authurl[d_opt]);
	printf("Using %s\n",cbuff);
  fflush(stdout);

	cc = curl_easy_setopt(curl, CURLOPT_URL, cbuff);
	if (cc != OK)
		return "Setopt ourl failed";

	curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
  curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
 	curl_easy_setopt(curl, CURLOPT_DEFAULT_PROTOCOL, "https");
 	curl_easy_setopt(curl, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_2_0);
  curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
  curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
  //curl_easy_setopt(curl, CURLOPT_PROXY, "https://192.168.7.2:8888");
#if 0		
									// "Postman-Token: ea87f94c-9296-4772-9c8c-8a9d2d6b5b3c");
	struct curl_slist *hd = curl_slist_append(NULL, g_post_m1);
	
	hd = curl_slist_append(hd, "cache-control: no-cache");
	hd = curl_slist_append(hd, 
	 "Authorization: Basic MmNsbmxlMnNyY3VvMmE3cWlxZHI0N2d1MXA6MTcwazJqanBwMHIzcGdmcGJjMWVzMmJwajhlOGFxOWdzZzZqN2hqZzhkbHFlZm5naHQydA==");
#else

//char ubuf[512];
//sprintf(ubuf, d_opt ? "client_id: %s" : "username: %s", g_client_id[d_opt]);
//char pbuf[512];
//sprintf(pbuf, d_opt ? "client_secret %s":"password: %s",g_client_secret[d_opt]);
//char abuf[512];
//sprintf(abuf, "Authorization: Bearer %s", g_authurl[d_opt]);

	struct curl_slist *hd = curl_slist_append((curl_slist *)NULL, 
                              "Content-Type: application/x-www-form-urlencoded");
	if (d_opt)
	{	// hd = curl_slist_append(hd, "Cookie: gateway-uk-eacc-entrypoint=1605033129.237.324.676852; AUTH_SESSION_ID=a5d31ab7-c1d0-4d58-8928-33398afc6437.at-sso-5448cb97ff-n9pww");
		// hd = curl_slist_append(hd,"Accept: */*");
		// hd = curl_slist_append(hd,"Cache-Control: no-cache");
	  // hd = curl_slist_append(hd,ubuf);
		// hd = curl_slist_append(hd,pbuf);
		// hd = curl_slist_append(hd,abuf);
		// hd = curl_slist_append(hd,"Cookie: gateway-uk-eacc-entrypoint=1604601692.884.324.141827");
		curl_easy_setopt(curl, CURLOPT_COOKIEFILE, ""); /* start cookie engine */ 
	}
	else
	{	/*hd=curl_slist_append(hd,ubuf);*/
  	/*hd=curl_slist_append(hd,pbuf);*/
	  //hd = curl_slist_append(hd,
//"Authorization: Basic M29qa2l1YTh0MjMwbmVxaGkxdGNudnR1djQ6bDltaWE1cGtnNmJpMmY1c2ptMjJ1M25yZm05ZjBpMDFkaHA1Zjd2aTVoaGRzNnByOWhi");
  }
#endif
  cc = curl_easy_setopt(curl, CURLOPT_HTTPHEADER, hd);
	if (cc != OK)
		return "Setopt httph failed";

	if (d_opt == 0)
	{	curl_easy_setopt(curl, CURLOPT_USERNAME, g_client_id[d_opt]);
		curl_easy_setopt(curl, CURLOPT_PASSWORD, g_client_secret[d_opt]);
	}

  const char * data = "";
	if (d_opt)
	{ char * buf = (char*)malloc(1000);
		sprintf(buf, "grant_type=client_credentials&client_secret=%s&client_id=%s",
								g_client_secret[d_opt], g_client_id[d_opt]);
		data = buf;
	}
  curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data);

  g_dd = (char*)malloc(1000);
  memset(g_dd, 0, 1000);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &g_dd);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curlWriteFunction);
//curl_easy_setopt(curl, CURLOPT_DEBUGFUNCTION, debug_callback);

//fprintf(stderr, "SE: Before Auth\n");
//printf("Before Auth\n");

	if (n_opt == 0)
	{ cc = curl_easy_perform(curl);
	
		const char * begtok = g_dd+2;
		while (*++begtok != 0 && *begtok != '"')
			;
		while (*++begtok != 0 && *begtok != '"')
			;
		begtok += 1;
		const char * endtok = begtok;
		while (*++endtok != 0 && *endtok != '"')
			;
//  *endtok = 0;
    if (endtok - begtok >= sizeof(g_access_token))
      printf("Token Too big (%d)\n",endtok - begtok);
    else
      memcpy(g_access_token, begtok, endtok - begtok);
    g_access_token[endtok - begtok] = 0;

  	if (v_opt)
  	{ printf("Giving %d %s\n", cc, g_access_token);
      fflush(stdout);

			CURLcode res;
			struct curl_slist *cookies;
			struct curl_slist *nc;
			int i;
		  fprintf(stderr, "Cookies, curl knows:\n");

			res = curl_easy_getinfo(curl, CURLINFO_COOKIELIST, &cookies);
			if (res != CURLE_OK) 
			{	fprintf(stderr, "Curl curl_easy_getinfo failed: %s\n",
			          curl_easy_strerror(res));
			  exit(1);
			}
			nc = cookies;
			i = 1;
			while (nc) 
			{ fprintf(stderr, "[%d]: %s\n", i, nc->data);

				char * nm = strstr(nc->data,"gateway-uk-eacc-entrypoint");
				if (nm != NULL)
				{	while (*++nm != 0 && *nm != 9)
						;
					if (*nm != 0)
						strpcpy(g_gw_cookie, nm+1, sizeof(g_gw_cookie)-1);
				}
			  nc = nc->next;
			  i++;
			}
			if(i == 1)
			  fprintf(stderr,"(none)\n");
		  curl_slist_free_all(cookies);
  	}
	}

	if (d_opt)
	{	
#if 0
		curl_easy_reset(curl);
#else
    curl_easy_cleanup(curl);
    curl_global_cleanup();
		sleep(1);         // make a pause if you working on console application
		curl = curl_easy_init();
	  g_curl = curl;
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &g_dd);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &curlWriteFunction);
#endif
		cc = curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "POST");
		if (cc != OK)
			return "Setopt cr failed";
	  cc = curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
		if (cc != OK)
			return "Setopt v failed";
	  curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
	  curl_easy_setopt(curl, CURLOPT_DEFAULT_PROTOCOL, "https");
		cc = curl_easy_setopt(curl, CURLOPT_COOKIEFILE, "");
		if (cc != OK)
			return "Setopt cf failed";
		cc = curl_easy_setopt(curl, CURLOPT_COOKIELIST, "SESS");
		if (cc != OK)
			return "Setopt cl failed";
		cc = curl_easy_setopt(curl, CURLOPT_COOKIE, "gateway-uk-eacc-entrypoint=");
		if (cc != OK)
			return "Setopt cook failed";
		cc = curl_easy_setopt(curl, CURLOPT_COOKIE, "OAuth_Token_Request_State=");
		if (cc != OK)
			return "Setopt cooka failed";
	}

	cc = curl_easy_setopt(curl, CURLOPT_URL, g_routerurl[d_opt]);
  if (cc != 0)
    return "Internal Error";

  return NULL;
}



Cc do_post(CURL * curl, char * rec)

{ char cbuff[512];
	int cc;
	if (v_opt || n_opt)
		printf("%s\n",rec);

	char b[MD5_DIGEST_LENGTH*2+4];
	char * cs = calc_md5(b,rec);
	strcpy(cbuff, "X-OriginalHash: "),
	strcat(cbuff, cs);
	if (p_opt)
	{ printf("Was %s\n", cbuff);
		strcpy(cbuff, "X-OriginalHash: 6d89a2ac3cc646035190fe0430baf00f");
	}

	if (v_opt)
		printf("%s\n", cbuff);
			
	if (n_opt != 0)
	  return OK;
	else
	{ char abuff[2048];
		sprintf(abuff,d_opt ? "Authorization: Bearer %s" 
												: "Authorization: %s", g_access_token);

//  char* data = malloc(2000); data[0] = 0; data[1] = 0; data[2] = 0; data[3]= 0;
//  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &data);
//  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &curlWriteFunction);

		struct curl_slist *hd = NULL;
		if (d_opt == 0)
		{	hd = curl_slist_append((curl_slist *)NULL,g_post_m2);
			hd = curl_slist_append(hd, "cache-control: no-cache");
			hd = curl_slist_append(hd, cbuff);
			hd = curl_slist_append(hd, abuff);
			hd = curl_slist_append(hd, "Content-Type: application/json");
		}
		else
		{ hd = curl_slist_append(hd, "Content-Type: application/json");
			hd = curl_slist_append(hd, abuff);
			char cook_buf[250];
			sprintf(cook_buf,"Cookie: gateway-uk-eacc-entrypoint=%s", g_gw_cookie);
			hd = curl_slist_append(hd, cook_buf);
		}
		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, hd);

		cc = curl_easy_setopt(curl, CURLOPT_POSTFIELDS, rec);
		g_data_size = 0;
    g_dd[0] = 0;

		cc = curl_easy_perform(curl);
		
		int instr = 0;
    char formatted[10000];
    int clamp = sizeof(formatted) - 1;
    char * s = g_dd - 1;
    char * t;
    for (t = formatted - 1; *++s != 0 && --clamp >= 0; )
    { if (*s == '"')
        instr = 1 - instr;
      if (!instr && *s == ',' && s[1] == '"')
        *++t = '\n';
      *++t = *s;
    }
    
    *++t = 0;
		printf("Response is %d %s\n",cc, *g_dd == 0 ? "<noresponse>" : formatted);
		fflush(stdout);

		return cc;
	}
}

		 
int do_get_uis(FILE * ip)

{ char row_buf[255];
	char * ln_;

	int wh = 0;
	int ct = 0;
	Tt_ui ttp = g_tt_uis - 1;

	while ((ln_ = fgets(row_buf, sizeof(row_buf) -1, ip)) != NULL)
	{ if (strlen(ln_) <= 4 
		 && ln_[0] == 'E' && ln_[1] == 'N' && ln_[2] == 'D')
			break;
		if (++ct > MAX_ITEMS)
			break;
		Char * s = ln_;
		wh |= *s == 'P' ? UI_PKT :
								'C' ? UI_AGG : 0;
		++ttp;
		ttp->wh = *s;
		while (*++s != ' ' && *s != 0)
			;
		ttp->price = atoi(s);
		*s = 0;
		strpcpy(&ttp->c[0], ln_+1, LUI);
	}
	g_tt_num_ui = ct;
			
	return wh;
}


void mk_head_eiv_erp(char * rec,
                     int inv_id,
              			 char date_[12],
                     const char * cust_eo,
                     const char * inv_tot,
                     int typ
                    )
{	char tim[20];
 	time_t rawtime;
 	time ( &rawtime );
 	struct tm * ptm = gmtime ( &rawtime );

  sprintf(tim,"%d%02d%02d12", ptm->tm_year-100, ptm->tm_mon+1, ptm->tm_mday);

  char date[20];  
  memcpy(date, date_, 12);
  if (date[0] == '?')
    sprintf(date,"%4d-%02d-%02d", ptm->tm_year+1900, ptm->tm_mon+1, ptm->tm_mday);
  char mt_time[20];
  sprintf(mt_time, "%02d:%02d:%02dZ", ptm->tm_hour, ptm->tm_min, ptm->tm_sec);

  if (inv_id <= 0)
    sprintf(rec,"{ \"EO_ID\": \"%s\","						 // 1
  									"\n    \"F_ID\": \"%s\"," 					  // 2
  									"\n    \"Event_Time\": \"%s\"," 	    // 3
  									"\n    \"Message_Time_Long\":\"%sT%s\"," // 4, 5
  									"\n    \"Product_Return\": 0," 
  									"\n    \"UI_Type\": %d,\n",           // 6
  								 g_our_eoid,g_our_fac_id,tim,date,mt_time, typ);
  else
    sprintf(rec,"{ \"EO_ID\": \"%s\","							// 1
							"\n    \"Event_Time\": \"%s\"," 			// 2
							"\n    \"Message_Time_Long\":\"%sT%s\"," // 3, 4
							"\n    \"Invoice_Type1\": 1,"
							"\n    \"Invoice_Number\": \"INV%d\"," // 5
							"\n    \"Invoice_Date\": \"%s\", "		 // 6
							"\n    \"Invoice_Seller\": \"%s\" ,"	 // 7
							"\n    \"Invoice_Buyer1\": 1,"
							"\n    \"Invoice_Buyer2\": \"%s%s\","  // 8, 9
							"\n    \"First_Seller_EU\": 0,"
							"\n    \"Invoice_Net\": %s,"					 // 10
							"\n    \"Invoice_Currency\": \"GBP\","
							"\n    \"UI_Type\": 2,\n",
  						g_our_eoid,tim,date,mt_time,inv_id,
  						 date,g_our_eoid,r_opt ? "" : g_eo_pfx, cust_eo,inv_tot);
//if (d_opt)
//	sprintf(&rec[strlen(rec)], "    \"Code\": \"INV%d\",\n", inv_id);
}


void mk_icv(char * rec,
            const char * cust_eo
           )
{ sprintf(rec,"{ \"Message Type\": \"ICV\""
  						"	 \"EO_IDS\":	[\"%s%s\"]" 						     // 1
  						"	 \"F_IDS\":	[]\n"
  						"	 \"M_IDS\":	[]\n"
  						"	 \"R_EOF\":	[]\n"
  						"	 \"R_EOFM\":	[]\n"
  						"	 \"Code\":	null\n",
  								 g_eo_pfx, cust_eo);
}


const char * tt_doit(const char * v_eo_id)

{ char date[12];
  time_t rawtime;
  time ( &rawtime );
  struct tm * ptm = gmtime (&rawtime);
  sprintf(date,"%d-%02d-%02d", ptm->tm_year+1900, ptm->tm_mon+1, ptm->tm_mday);

  const char * res = do_request_access();
  if (res != NULL)
    return res;

  CURL * curl = g_curl;
	int max_rec = 400000;
	if (g_rec == NULL)
	  g_rec = (char*)malloc(max_rec + 1);
	char * rec = g_rec;

  if (v_eo_id != NULL)              // Only verify eo_id
  { mk_head_eiv_erp(rec, 1, date, v_eo_id, "0.00", UI_PKT);
    strcat(rec, "\"upUIs\":[\"010500043102597421MS84ZQMK8FZ924\"],");
		strcat(rec, "\n    \"Message_Type\": \"EIV\"\n}\n");

		Cc cc  = do_post(curl, rec);
		if (g_dd[0] != 0)
		{ char * d = strmatch("EO_IDS_EXIST", g_dd);
		  if (d[0] == 0)
		    if (d[16] == 't')
		      return NULL;        // "EO_IDS_EXIST": [true, true],
		}
		return "Not Exist";
  }
  else
  { global_lock();
  	if (k_opt + n_opt == 0)
  		system("mv tt_inv/tt_list.dat /tmp/tt_list1234567.dat");
  	else
  		system("cp tt_inv/tt_list.dat /tmp/tt_list1234567.dat");
  	global_unlock();

  	char inv_buf[64];
  	FILE * inv_f = fopen("/tmp/tt_list1234567.dat", "r");
  	if (inv_f != 0) 														 // EIV
  	{
  //	CURL *curl = curl_easy_init();
  //	cc = curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "POST");
  		while (1)
  		{ char inv_buf[255];
  			char * ln = fgets(inv_buf, sizeof(inv_buf) -1, inv_f);
  			if (ln == NULL)
  				break;

  			int in_inv_id = atoi(inv_buf);
  			if (in_inv_id == 0)
  				break;
  				
  			if (a_opt && in_inv_id > 0)
  			  continue;

  			char fnbuff[256];
  			char * fname = prepare_dfile(&fnbuff[0], "tt_inv", in_inv_id);
  			if (fname == NULL)
  				continue; 		// No logging!
  			
  			FILE * ip = fopen(fname, "r");			// dont bother with locking!
  			if (ip == NULL)
  				continue;

        if (--g_clamp < 0)
          continue;

        if (v_opt)
          printf("Invoice %d\n",in_inv_id);

  			char row_buf[255];
  			char * ln_ = fgets(row_buf, sizeof(row_buf) -1, ip);
  			if (ln_ == NULL)
  				continue;

  			int inv_id, sz;
  			char cust_eo_id[41], fac_id[41];
        char wh[4], canc[6];
  			char inv_tot[14]; inv_tot[0] = 0;
  			int c_id = 0;
				if (strcmp("DNOTE", ln_) == 0)
				{ char buf[60];
  				int n = sscanf(ln_, "DNOTE %s Date %s", buf, date);
        	mk_head_eiv_erp(rec, 0, date, "", "", UI_AGG);
    			strcat(strcat(rec, "    \"aUIs\": ["),buf);
  				strcat(rec, "],\n");
				}
				else 		
  			{ int n = sscanf(ln_, 
  										"%3s %d Size %d EO_ID %40[^,],%40s %5s Total %13s Date %11s",
  										 &wh,&inv_id, &sz, cust_eo_id, fac_id, &canc, &inv_tot, date);

  				int ret = canc[0] == '.' ? 1 : 2;
  			
	  			char * cid_str = strstr(ln_, "Cid");
  				if (cid_str != NULL)
  				  c_id = atoi(cid_str +3);

      	  if (r_opt)
       		{ strcpy(cust_eo_id, g_eoids[(g_eoid_ix++) % 3]);
       	  	strcpy(fac_id, g_fids[g_eoid_ix % 3]);
        	}

	  			if (n < 5)
				  	continue;

			  	if (inv_id >= 0 && !in_range(toupper(cust_eo_id[0]),'A','Z'))
  					continue;

	        if (ill_eids.got_eid(cust_eo_id))
	        { printf("Suppressed %s\n", cust_eo_id);
	          continue;
	        }
  			
	  			int typ = do_get_uis(ip);

	        mk_head_eiv_erp(rec, inv_id, date, cust_eo_id, inv_tot, typ);

	        int num_prod = 0;

	        for (int which = UI_PKT - 1; ++which <= UI_AGG; )
	        {
	    			if ((typ & which) == 0)
	            continue;
	    			strcat(rec, /* which & UI_PKT ? "    \"upUIs\":["
                                     : */ "    \"aUIs\": [");
  	        int got_ele = 0;
    				int pkt_ix = -1;
    				Tt_ui ttp = g_tt_uis - 1;
	    			while (++pkt_ix < g_tt_num_ui)
	    			{ ++ttp;
  	  			  int ty = ttp->wh == 'P' ? UI_PKT : UI_AGG;
	    				if ((ty & typ) == 0)
  	  					continue;
	          { char ubuff[132];
	            char * t = ubuff - 1;
	            char * s = ttp->c;
	            if (s[0] == '0' && s[1] == '1')
	            { s += 2;
	              int ct = 14;
	              while (--ct >= 0 && *s != 0)
	                *++t = *s++;
	              if (s[0] == '2' && s[1] == '1')
	                s += 2;
	              ct = 19;  // was 17
	              int min_ct = 10; // say
	              while (--ct >= 0 && *s != 0 
	                   &&( --min_ct >= 0 || *s != '2' || s[1] != '4' || 
	                                        s[2] != '0' && s[2] != '1'
	                                  ))
	                *++t = *s++;
	              if (e_opt && *s != 0)
	              { s += 2;
	                while (*++s != 0)
	                  *++t = *s;
	              }
	              *++t = 0;
	              s = ubuff;
	            }
//          0500039336488376350FK1934256733
//          05060592002856SUW3ZG690C1X
	            if (strlen(s) <= g_longest_ui)
	            { ++num_prod;
	              if (got_ele++ > 0)
	                strcat(rec, "             ");
	      				strcat(strcat(strcat(rec,"\""),s),"\",\n");
	      			}
  	  			}}
					}
          if (rec[strlen(rec)-1] == '\n')
    				rec[strlen(rec)-2] = 0;
  				strcat(rec, "],\n");
  			}
  			
  			if (num_prod <= 0)
  			  continue;

  			sprintf(rec+strlen(rec), "\n    \"Message_Type\": \"%s\"\n}\n",
  			          inv_id < 0 ? "ERP" : "EIV"
  			       );
  			       
  			Cc cc = do_post(curl, rec);

        if (g_err_log == NULL)
          g_err_log = fopen("tt_err_log","a+");

        if (g_err_log != NULL)
        { char * eo_id = strmatch("EOID_NOT_EXIST_OR_ACTIVE", g_dd);
          if (eo_id[0] == 0)
          { fprintf(g_err_log, "EOID %s not recognised (Inv %d)(Cust %d)\n", 
                               cust_eo_id, inv_id, c_id);
            fflush(g_err_log);
          }
        }

  			if (t_opt)
  			{ sprintf(rec,
  "{\n    \"EO_ID\": \"ONB_EO144IB3002078572YSHREJCOLW6316418352RCNUE0298\","
  "\n    \"Event_Time\": \"19041113\","
  "\n    \"Invoice_Type1\": 1,"
  "\n    \"Invoice_Type2\": \"other type\","
  "\n    \"Invoice_Number\": \"INV000001\","
  "\n    \"Invoice_Date\": \"2019-04-11\","
  "\n    \"Invoice_Seller\": \"Seller_ID\","
  "\n    \"Invoice_Buyer1\": 1,"
  "\n    \"Buyer_CountryReg\": \"BG\","
  "\n    \"Invoice_Buyer2\": \"ONB_EO144IB3002078572YSHREJCOLW6316418352RCNUE0298\","
  "\n    \"Buyer_Address\": \"Buyer_Address\","
  "\n    \"Buyer_TAX_N\": \"TAX0001\","
  "\n    \"First_Seller_EU\": 1,"
  "\n    \"Product_Items_1\": ["
  "\n     	 \"5cd2729e-6acc-4479-b67e-a26a84a6e88b\","
  "\n     	 \"752a77ae-d2a3-4c47-bc92-6a40bd2e6ef3\""
  "\n    ],"
  "\n    \"Product_Items_2\": ["
  "\n     	 \"5cd2729e-6acc-4479-b67e-a26a84a6e88b\","
  "\n     	 \"752a77ae-d2a3-4c47-bc92-6a40bd2e6ef3\""
  "\n    ],"
  "\n    \"Product_Price\": ["
  "\n     	 16.99,"
  "\n     	 19.99"
  "\n    ],"
  "\n    \"Invoice_Net\": 10.99,"
  "\n    \"Invoice_Currency\": \"EUR\","
  "\n    \"UI_Type\": 1,"
  "\n    \"upUIs\": ["
  "\n     	 \"2439092f-c460-4140-8ce4-e07069fc89fe19030511\""
  "\n    ],"
  "\n    \"aUIs\": [],"
  "\n    \"Invoice_comment\": \"Ivoice_Comment\","
  "\n    \"Message_Type\": \"EIV\""
  "\n}");
  				(void)do_post(curl, rec);
  			}
  		} // over invoices
  		system("rm /tmp/tt_list1234567.dat");
  	}
  }

	if (g_curl != 0)
		curl_easy_cleanup(g_curl);
	curl_global_cleanup();

  return res;
}

#ifndef TT_LIB

void explain()

{ fprintf(stderr, "tt_route <opts>\n"
									"      -a      -- Accept first\n"
									"      -d      -- use DeLaRue\n"
									"      -D      -- use DeLaRue production\n"
									"      -c eoid -- Check EO_ID\n"
									"      -k      -- Keep send list\n"
									"      -l      -- Dont exclude illegal EO_IDs\n"
									"      -m #    -- Max invoices to send\n"
									"      -n      -- dont send\n"
									"      -v      -- verbose\n");
	exit(0);
}


int main(int argc, char**argv)

{
	int argsleft = argc - 1;
	char ** argv_ = &argv[1];

	for (; argsleft > 0 and argv_[0][0] == '-'; --argsleft)
	{ Char * flgs;
		for (flgs = &argv_[0][1]; *flgs != 0; ++flgs)
			if			(*flgs == 'a')
				a_opt = 1;
			else if (*flgs == 'b')    // not in use
				b_opt = 1;
			else if (*flgs == 'c')
			{ if (--argsleft == 0) 
			    explain();
			  ++argv_;
			  g_check_eoid = argv_[0];
			}
			else if (*flgs == 'd')
				d_opt = 1;
			else if (*flgs == 'D')
				d_opt = 2;
			else if (*flgs == 'e')
				e_opt = 1;
			else if (*flgs == 'k')
				k_opt = 1;
			else if (*flgs == 'l')
				l_opt = 1;
			else if (*flgs == 'm')
			{ if (--argsleft == 0) 
			    explain();
			  ++argv_;
				g_clamp = atoi(argv_[0]);
				if (g_clamp <= 0)
			    explain();
			}
			else if (*flgs == 'z')
			{ if (--argsleft == 0) 
			    explain();
			  ++argv_;
				g_longest_ui  = atoi(argv_[0]);
				if (g_longest_ui <= 0)
			    explain();
			}
			else if (*flgs == 'n')
				n_opt = 1;
			else if (*flgs == 'p')
				p_opt = 1;
			else if (*flgs == 'r')
				r_opt = 1;
			else if (*flgs == 't')
				t_opt = 1;
			else if (*flgs == 'v')
				v_opt = 1;
			else if (*flgs == 'P')
      { char buff[132];
        char * fn = prepare_dfile(buff, "tt_inv", -4);
        printf("Res %s\n", fn == NULL ? "Null" : fn);
        explain();
      }
			else
				explain();
		++argv_;
	}

	char * authurl_env = getenv("TT_ROUTER");
	if (authurl_env != NULL)
		g_authurl[d_opt] = authurl_env;

	char * our_eoid = getenv("TT_EOID");
	if (our_eoid != NULL)
		strpcpy(g_our_eoid, our_eoid, sizeof(g_our_eoid) - 1);

  g_dbg_file = stderr;

	if (l_opt == 0)
	  ill_eids.load_eids();

{ const char * res = tt_doit(g_check_eoid);
  if      (res != NULL)
	{ if (g_check_eoid == NULL)
  	  printf("%s\n", res);
  	else
	    printf("%s %s%s\n", res, g_eo_pfx,g_check_eoid);

		exit(1);
	}
	else if (g_check_eoid != NULL)
	  printf("Valid EO_ID %s\n", g_check_eoid);
	
	debug_callback(NULL);

	exit(0);
}}

#else

char g_resp_buff[10000];

Cc tt_check_eo_id(const char * eo_id)

{ if (g_dbg_file == NULL)
    g_dbg_file  = fopen(g_dbg_file_name, "a+");

  char * res = tt_doit(eo_id);

  if (g_dbg_file != NULL)
    fclose(g_dbg_file);

  return res == NULL ? OK : -1;
}

char tt_send_invs()

{ g_dbg_file = stderr;

  g_resp_buf[0] = 0;
  char * res = tt_doit(eo_id);
  if (res != NULL)
    strpcpy(g_resp_buff, res, sizeof(g_resp_buff));

  g_dbg_file = NULL;
  return g_resp_buff;
}

#endif
