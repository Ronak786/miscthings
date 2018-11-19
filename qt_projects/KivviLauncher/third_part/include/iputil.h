/*************************************************************************
	> File Name: iputil.h
	> Author: davee-x
	> Mail: davee.naughty@gmail.com 
	> Created Time: Wed 13 Jun 2018 04:03:15 PM CST
 ************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

int lte_open_modem();
int lte_get_siglevel();
int lte_close_modem();

#ifdef __cplusplus
}
#endif

