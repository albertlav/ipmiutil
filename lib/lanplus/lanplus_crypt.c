/*
 * Copyright (c) 2003 Sun Microsystems, Inc.  All Rights Reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 
 * Redistribution of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 * 
 * Redistribution in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 * 
 * Neither the name of Sun Microsystems, Inc. or the names of
 * contributors may be used to endorse or promote products derived
 * from this software without specific prior written permission.
 * 
 * This software is provided "AS IS," without a warranty of any kind.
 * ALL EXPRESS OR IMPLIED CONDITIONS, REPRESENTATIONS AND WARRANTIES,
 * INCLUDING ANY IMPLIED WARRANTY OF MERCHANTABILITY, FITNESS FOR A
 * PARTICULAR PURPOSE OR NON-INFRINGEMENT, ARE HEREBY EXCLUDED.
 * SUN MICROSYSTEMS, INC. ("SUN") AND ITS LICENSORS SHALL NOT BE LIABLE
 * FOR ANY DAMAGES SUFFERED BY LICENSEE AS A RESULT OF USING, MODIFYING
 * OR DISTRIBUTING THIS SOFTWARE OR ITS DERIVATIVES.  IN NO EVENT WILL
 * SUN OR ITS LICENSORS BE LIABLE FOR ANY LOST REVENUE, PROFIT OR DATA,
 * OR FOR DIRECT, INDIRECT, SPECIAL, CONSEQUENTIAL, INCIDENTAL OR
 * PUNITIVE DAMAGES, HOWEVER CAUSED AND REGARDLESS OF THE THEORY OF
 * LIABILITY, ARISING OUT OF THE USE OF OR INABILITY TO USE THIS SOFTWARE,
 * EVEN IF SUN HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.
 */

#include <assert.h>
#include <string.h>
#if defined(HAVE_CONFIG_H)
# include <config.h>
#endif
#include <ipmitool/bswap.h>
#include <ipmitool/log.h>
#include "lanplus.h"
#include "lanplus_crypt.h"
#include "lanplus_crypt_impl.h"

/*
 * lanplus_rakp2_hmac_matches
 * 
 * param session holds all the state data that we need to generate the hmac
 * param hmac is the HMAC sent by the BMC in the RAKP 2 message
 *
 * The HMAC was generated [per RFC2404] from : 
 * 
 *     SIDm     - Remote console session ID    
 *     SIDc     - BMC session ID
 *     Rm       - Remote console random number
 *     Rc       - BMC random number
 *     GUIDc    - BMC guid
 *     ROLEm    - Requested privilege level (entire byte)
 *     ULENGTHm - Username length
 *     <UNAMEm> - Username (absent for null user names)
 *
 * generated by using Kuid.  I am aware that the subscripts on the values
 * look backwards, but that's the way they are written in the specification.
 *
 * If the authentication algorithm is IPMI_AUTH_RAKP_NONE, we return success.
 * 
 * return 0 on success (the authcode matches)
 *        1 on failure (the authcode does not match)
 */
int lanplus_rakp2_hmac_matches(const struct ipmi_session * session,
							   const uint8_t    * bmc_mac,
							   struct ipmi_intf * intf)
{
	uint8_t       * buffer;
	int           bufferLength, i;
	uint8_t       mac[EVP_MAX_MD_SIZE];
	uint32_t      macLength;

	uint32_t SIDm_lsbf, SIDc_lsbf;

	if (session->v2_data.auth_alg == IPMI_AUTH_RAKP_NONE)
		return 1;
	
	/* We don't yet support other algorithms (was assert) */
	if ((session->v2_data.auth_alg != IPMI_AUTH_RAKP_HMAC_SHA1) &&
	    (session->v2_data.auth_alg != IPMI_AUTH_RAKP_HMAC_MD5)  &&
	    (session->v2_data.auth_alg != IPMI_AUTH_RAKP_HMAC_SHA256)) {
		printf("Error, unsupported rakp2 auth alg %d\n",
			session->v2_data.auth_alg);
		return 1;
	}

	bufferLength =
		4  +                       /* SIDm     */
		4  +                       /* SIDc     */
		16 +                       /* Rm       */
		16 +                       /* Rc       */
		16 +                       /* GUIDc    */
		1  +                       /* ROLEm    */
		1  +                       /* ULENGTHm */
		(int)strlen((const char *)session->username); /* optional */

	buffer = malloc(bufferLength);
	if (buffer == NULL) {
		lprintf(LOG_ERR, "lanplus: malloc failure");
		return 1;
	}

	/*
	 * Fill the buffer.  I'm assuming that we're using the LSBF representation of the
	 * multibyte numbers in use.
	 */

	/* SIDm */
	SIDm_lsbf = session->v2_data.console_id;
	#if WORDS_BIGENDIAN
	SIDm_lsbf = BSWAP_32(SIDm_lsbf);
	#endif

	memcpy(buffer, &SIDm_lsbf, 4);

	/* SIDc */
	SIDc_lsbf = session->v2_data.bmc_id;
	#if WORDS_BIGENDIAN
	SIDc_lsbf = BSWAP_32(SIDc_lsbf);
	#endif
	memcpy(buffer + 4, &SIDc_lsbf, 4);

	/* Rm */
	#if WORDS_BIGENDIAN
	for (i = 0; i < 16; ++i)
		buffer[8 + i] = session->v2_data.console_rand[16 - 1 - i];
	#else
	for (i = 0; i < 16; ++i)
		buffer[8 + i] = session->v2_data.console_rand[i];
	#endif

	/* Rc */
	#if WORDS_BIGENDIAN
	for (i = 0; i < 16; ++i)
		buffer[24 + i] = session->v2_data.bmc_rand[16 - 1 - i];
	#else
	for (i = 0; i < 16; ++i)
		buffer[24 + i] = session->v2_data.bmc_rand[i];
	#endif

	/* GUIDc */
	#if WORDS_BIGENDIAN
	for (i = 0; i < 16; ++i)
		buffer[40 + i] = session->v2_data.bmc_guid[16 - 1 - i];
	#else
	for (i = 0; i < 16; ++i)
		buffer[40 + i] = session->v2_data.bmc_guid[i];
	#endif

	/* ROLEm */
	buffer[56] = session->v2_data.requested_role;

	/* ULENGTHm */
	buffer[57] = (uint8_t)strlen((const char *)session->username);

	/* UserName [optional] */
	for (i = 0; i < buffer[57]; ++i)
		buffer[58 + i] = session->username[i];

	if (verbose > 2)
	{
		lprintf(LOG_DEBUG,"rakp2 mac input buffer (%d bytes)", bufferLength);
	}

	/*
	 * The buffer is complete.  Let's hash.
	 */
	lanplus_HMAC(session->v2_data.auth_alg,
				 session->authcode,
				 IPMI_AUTHCODE_BUFFER_SIZE,
				 buffer,
				 bufferLength,
				 mac,
				 &macLength);

	free(buffer);


	if (verbose > 2)
	{
		printbuf(mac, macLength, ">> rakp2 mac as computed by the remote console");
	}

	return (memcmp(bmc_mac, mac, macLength) == 0);
}



/*
 * lanplus_rakp4_hmac_matches
 * 
 * param session holds all the state data that we need to generate the hmac
 * param hmac is the HMAC sent by the BMC in the RAKP 4 message
 *
 * The HMAC was generated [per RFC2404] from : 
 * 
 *     Rm       - Remote console random number
 *     SIDc     - BMC session ID
 *     GUIDc    - BMC guid
 *
 * generated by using SIK (the session integrity key).  I am aware that the
 * subscripts on the values look backwards, but that's the way they are
 * written in the specification.
 *
 * If the authentication algorithm is IPMI_AUTH_RAKP_NONE, we return success.
 * 
 * return 1 on success (the authcode matches)
 *        0 on failure (the authcode does not match)
 *
 */
int lanplus_rakp4_hmac_matches(const struct ipmi_session * session,
							   const uint8_t    * bmc_mac,
							   struct ipmi_intf * intf)
{
	uint8_t       * buffer;
	int           bufferLength, i;
	uint8_t       mac[EVP_MAX_MD_SIZE];
	uint32_t      macLength;
	uint32_t      cmpLength;
	uint32_t      SIDc_lsbf;
	int           unsupported = 0;
	uint8_t       alg;

	if (ipmi_oem_active(intf, "intelplus")){
		/* Intel BMC responds with the integrity Algorithm in RAKP4 */
		if (session->v2_data.integrity_alg == IPMI_INTEGRITY_NONE)
			return 1;

		/* (Old) Intel BMC doesn't support other algorithms */
		if ((session->v2_data.integrity_alg != IPMI_INTEGRITY_HMAC_SHA1_96) &&
		    (session->v2_data.integrity_alg != IPMI_INTEGRITY_HMAC_MD5_128)) {
			printf("Error, unsupported rakp4 integrity_alg %d\n",
				session->v2_data.integrity_alg);
			return 1;
		}
	} else {
		if (session->v2_data.auth_alg == IPMI_AUTH_RAKP_NONE)
			return 1;

		/* We don't yet support other algorithms (was assert) */
		if ((session->v2_data.auth_alg != IPMI_AUTH_RAKP_HMAC_SHA1) &&
	  	    (session->v2_data.auth_alg != IPMI_AUTH_RAKP_HMAC_MD5)  &&
		    (session->v2_data.auth_alg != IPMI_AUTH_RAKP_HMAC_SHA256)) {
			printf("Error, unsupported rakp4 auth alg %d\n",
				session->v2_data.auth_alg);
			return 1;
		}
	}

	bufferLength =
		16 +   /* Rm    */
		4  +   /* SIDc  */
		16;    /* GUIDc */

	buffer = (uint8_t *)malloc(bufferLength);
	if (buffer == NULL) {
		lprintf(LOG_ERR, "lanplus: malloc failure");
		return 1;
	}

	/*
	 * Fill the buffer.  I'm assuming that we're using the LSBF 
	 * representation of the multibyte numbers in use.
	 */

	/* Rm */
	#if WORDS_BIGENDIAN
	for (i = 0; i < 16; ++i)
		buffer[i] = session->v2_data.console_rand[16 - 1 - i];
	#else
	for (i = 0; i < 16; ++i)
		buffer[i] = session->v2_data.console_rand[i];
	#endif


	/* SIDc */
	SIDc_lsbf = session->v2_data.bmc_id;
	#if WORDS_BIGENDIAN
	SIDc_lsbf = BSWAP_32(SIDc_lsbf);
	#endif
	memcpy(buffer + 16, &SIDc_lsbf, 4);
	

	/* GUIDc */
	#if WORDS_BIGENDIAN
	for (i = 0; i < 16; ++i)
		buffer[i +  20] = session->v2_data.bmc_guid[16 - 1 - i];
	#else
	for (i = 0; i < 16; ++i)
		buffer[i +  20] = session->v2_data.bmc_guid[i];
	#endif


	if (verbose > 2)
	{
		printbuf((const uint8_t *)buffer, bufferLength, ">> rakp4 mac input buffer");
		printbuf(session->v2_data.sik, session->v2_data.sik_len, ">> rakp4 mac key (sik)");
	}


	/*
	 * The buffer is complete.  Let's hash.
	 */
	alg = ( (ipmi_oem_active(intf, "intelplus")) 
	             ? session->v2_data.integrity_alg 
	             : session->v2_data.auth_alg );
	lanplus_HMAC(alg, 
				 session->v2_data.sik,
				 session->v2_data.sik_len,
				 buffer,
				 bufferLength,
				 mac,
				 &macLength);

	if (verbose > 2)
	{
		printbuf(bmc_mac, macLength, ">> rakp4 mac as computed by the BMC");
		printbuf(mac,     macLength, ">> rakp4 mac as computed by the remote console");
	}

	if (ipmi_oem_active(intf, "intelplus")){
		/* Intel BMC responds with the integrity Algorithm in RAKP4 */
		switch(session->v2_data.integrity_alg)
		{
		case IPMI_INTEGRITY_HMAC_SHA1_96: 
			if (macLength != SHA_DIGEST_LENGTH) unsupported = 1; 
			cmpLength = IPMI_SHA1_AUTHCODE_SIZE; 
			break;
		case IPMI_INTEGRITY_HMAC_MD5_128: 
			if (macLength != MD5_DIGEST_LENGTH) unsupported = 1;
			cmpLength = IPMI_HMAC_MD5_AUTHCODE_SIZE; 
			break;
		default: 
			unsupported = 1;
			break;
		}

	} else {

		/* We don't yet support other algorithms (was assert) */
		switch(session->v2_data.auth_alg)
		{
		case IPMI_AUTH_RAKP_HMAC_SHA1: 
			if (macLength != SHA_DIGEST_LENGTH) unsupported = 1; 
			cmpLength = IPMI_SHA1_AUTHCODE_SIZE; 
			break;
		case IPMI_AUTH_RAKP_HMAC_MD5 : 
			if (macLength != MD5_DIGEST_LENGTH) unsupported = 1; 
			cmpLength = IPMI_HMAC_MD5_AUTHCODE_SIZE; 
			break;
		case IPMI_AUTH_RAKP_HMAC_SHA256: 
			if (macLength != SHA256_DIGEST_LENGTH) unsupported = 1; 
			cmpLength = IPMI_HMAC_SHA256_AUTHCODE_SIZE; 
			break;
		default:
			unsupported = 1;
			break;
		} 
	}
	if (unsupported) {
	   printf("Unsupported rakp4 macLength %d for auth %d\n",
			macLength, session->v2_data.auth_alg);
	   return 1;
	}

	free(buffer);
	return (memcmp(bmc_mac, mac, cmpLength) == 0);
}



/*
 * lanplus_generate_rakp3_auth_code
 *
 * This auth code is an HMAC generated with :
 *
 *     Rc         - BMC random number
 *     SIDm       - Console session ID
 *     ROLEm      - Requested privilege level (entire byte)
 *     ULENGTHm   - Username length
 *     <USERNAME> - Usename (absent for null usernames)
 *
 * The key used to generated the MAC is Kuid
 *
 * I am aware that the subscripts look backwards, but that is the way they are
 * written in the spec.
 * 
 * param output_buffer [out] will hold the generated MAC
 * param session       [in]  holds all the state data we need to generate the auth code
 * param mac_length    [out] will be set to the length of the auth code
 *
 * returns 0 on success
 *         1 on failure
 */
int lanplus_generate_rakp3_authcode(uint8_t                      * output_buffer,
									const struct ipmi_session * session,
									uint32_t                  * mac_length,
									struct ipmi_intf          * intf)
{
	int ret = 0;
	int input_buffer_length, i;
	uint8_t * input_buffer;
	uint32_t SIDm_lsbf;
	

	if (session->v2_data.auth_alg == IPMI_AUTH_RAKP_NONE)
	{
		*mac_length = 0;
		return 0;
	}

	/* We don't yet support other algorithms (was assert) */
	if ((session->v2_data.auth_alg != IPMI_AUTH_RAKP_HMAC_SHA1) &&
	    (session->v2_data.auth_alg != IPMI_AUTH_RAKP_HMAC_MD5)  &&
	    (session->v2_data.auth_alg != IPMI_AUTH_RAKP_HMAC_SHA256)) {
		printf("Error, unsupported rakp3 auth alg %d\n",
			session->v2_data.auth_alg);
		return 1;
	}

	input_buffer_length =
		16 + /* Rc       */
		4  + /* SIDm     */
		1  + /* ROLEm    */
		1  + /* ULENGTHm */
		(int)strlen((const char *)session->username);

	input_buffer = malloc(input_buffer_length);
	if (input_buffer == NULL) {
		lprintf(LOG_ERR, "lanplus: malloc failure");
		return 1;
	}

	/*
	 * Fill the buffer.  I'm assuming that we're using the LSBF representation of the
	 * multibyte numbers in use.
	 */
	
	/* Rc */
	#if WORDS_BIGENDIAN
	for (i = 0; i < 16; ++i)
		input_buffer[i] = session->v2_data.bmc_rand[16 - 1 - i];
	#else
	for (i = 0; i < 16; ++i)
		input_buffer[i] = session->v2_data.bmc_rand[i];
	#endif

	/* SIDm */
	SIDm_lsbf = session->v2_data.console_id;
	#if WORDS_BIGENDIAN
	SIDm_lsbf = BSWAP_32(SIDm_lsbf);
	#endif
	memcpy(input_buffer + 16, &SIDm_lsbf, 4);
	
	/* ROLEm */
	if (ipmi_oem_active(intf, "intelplus")) 
		input_buffer[20] = session->privlvl;
	else 
		input_buffer[20] = session->v2_data.requested_role;

	/* ULENGTHm */
	input_buffer[21] = (uint8_t)strlen((const char *)session->username);

	/* USERNAME */
	for (i = 0; i < input_buffer[21]; ++i)
		input_buffer[22 + i] = session->username[i];

	if (verbose > 2)
	{
		printbuf((const uint8_t *)input_buffer, input_buffer_length, ">> rakp3 mac input buffer");
		printbuf((const uint8_t *)session->authcode, IPMI_AUTHCODE_BUFFER_SIZE, ">> rakp3 mac key");
	}
    
	lanplus_HMAC(session->v2_data.auth_alg,
				 session->authcode,
				 IPMI_AUTHCODE_BUFFER_SIZE,
				 input_buffer,
				 input_buffer_length,
				 output_buffer,
				 mac_length);

	if (verbose > 2)
		printbuf((const uint8_t *)output_buffer, *mac_length, "generated rakp3 mac");

	
	free(input_buffer);

	return ret;
}



/*
 * lanplus_generate_sik
 *
 * Generate the session integrity key (SIK) used for integrity checking 
 * during the IPMI v2 / RMCP+ session
 *
 * This session integrity key is a HMAC generated with :
 *
 *     Rm         - Console generated random number
 *     Rc         - BMC generated random number
 *     ROLEm      - Requested privilege level (entire byte)
 *     ULENGTHm   - Username length
 *     <USERNAME> - Usename (absent for null usernames)
 *
 * The key used to generated the SIK is Kg if Kg is not null (two-key logins are
 * enabled).  Otherwise Kuid (the user authcode) is used as the key to genereate
 * the SIK.
 *
 * I am aware that the subscripts look backwards, but that is the way they are
 * written in the spec.
 * 
 * param session [in/out] contains our input and output fields.
 *
 * returns 0 on success
 *         1 on failure
 */
int lanplus_generate_sik(struct ipmi_session * session)
{
	uint8_t * input_buffer;
	int input_buffer_length, i;
	uint8_t * input_key;
	uint32_t mac_length;
	int unsupported = 0;

	memset(session->v2_data.sik, 0, sizeof(session->v2_data.sik));
	session->v2_data.sik_len = 0;

	if (session->v2_data.auth_alg == IPMI_AUTH_RAKP_NONE)
		return 0;

	/* We don't yet support other algorithms (was assert) */
	if ((session->v2_data.auth_alg != IPMI_AUTH_RAKP_HMAC_SHA1) &&
	    (session->v2_data.auth_alg != IPMI_AUTH_RAKP_HMAC_MD5)  &&
	    (session->v2_data.auth_alg != IPMI_AUTH_RAKP_HMAC_SHA256)) {
		printf("Error, unsupported sik auth alg %d\n",
			session->v2_data.auth_alg);
		return 1;
	}

	input_buffer_length =
		16 +  /* Rm       */
		16 +  /* Rc       */
		1  +  /* ROLEm     */
		1  +  /* ULENGTHm  */
		(int)strlen((const char *)session->username);

	input_buffer = malloc(input_buffer_length);
	if (input_buffer == NULL) {
		lprintf(LOG_ERR, "lanplus: malloc failure");
		return 1;
	}

	/*
	 * Fill the buffer.  I'm assuming that we're using the LSBF representation of the
	 * multibyte numbers in use.
	 */

	/* Rm */
	#if WORDS_BIGENDIAN
	for (i = 0; i < 16; ++i)
		input_buffer[i] = session->v2_data.console_rand[16 - 1 - i];
	#else
	for (i = 0; i < 16; ++i)
		input_buffer[i] = session->v2_data.console_rand[i];
	#endif
	

	/* Rc */
	#if WORDS_BIGENDIAN
	for (i = 0; i < 16; ++i)
		input_buffer[16 + i] = session->v2_data.bmc_rand[16 - 1 - i];
	#else
	for (i = 0; i < 16; ++i)
		input_buffer[16 + i] = session->v2_data.bmc_rand[i];
	#endif

	/* ROLEm */
	input_buffer[32] = session->v2_data.requested_role;

	/* ULENGTHm */
	input_buffer[33] = (uint8_t)strlen((const char *)session->username);

	/* USERNAME */
	for (i = 0; i < input_buffer[33]; ++i)
		input_buffer[34 + i] = session->username[i];

	if (session->v2_data.kg[0])
	{
		/* We will be hashing with Kg */
		/*
		 * Section 13.31 of the IPMI v2 spec describes the SIK creation
		 * using Kg.  It specifies that Kg should not be truncated.
		 * Kg is set in ipmi_intf.
		 */
		input_key        = session->v2_data.kg;
	}
	else
	{
		/* We will be hashing with Kuid */
		input_key        = session->authcode;
	}

	
	if (verbose >= 2)
		printbuf((const uint8_t *)input_buffer, input_buffer_length, "session integrity key input");

	lanplus_HMAC(session->v2_data.auth_alg,
				 input_key,
				 IPMI_AUTHCODE_BUFFER_SIZE,
				 input_buffer,
				 input_buffer_length,
				 session->v2_data.sik,
				 &mac_length);

	free(input_buffer);
	switch(session->v2_data.auth_alg)
	{
	case IPMI_AUTH_RAKP_HMAC_SHA1  : 
		if (mac_length != SHA_DIGEST_LENGTH) unsupported = 1; 
		break;
	case IPMI_AUTH_RAKP_HMAC_MD5   : 
		if (mac_length != MD5_DIGEST_LENGTH) unsupported = 1; 
		break;
	case IPMI_AUTH_RAKP_HMAC_SHA256: 
		if (mac_length != SHA256_DIGEST_LENGTH) unsupported = 1; 
		break;
	default                        : unsupported = 1; break;
	}
	if (unsupported) {  /*was assert*/
	   printf("Unsupported sik macLength %d for auth %d\n",
			mac_length, session->v2_data.auth_alg);
	   return 1;
	}
	
	session->v2_data.sik_len = (uint8_t)mac_length;

	/*
	 * The key MAC generated is 20 bytes, but we will only be using the first
	 * 12 for SHA1 96
	 */
	if (verbose >= 2)
		printbuf(session->v2_data.sik, session->v2_data.sik_len, "Generated session integrity key");

	return 0;
}



/*
 * lanplus_generate_k1
 *
 * Generate K1, the key presumably used to generate integrity authcodes
 *
 * We use the authentication algorithm to generated the HMAC, using
 * the session integrity key (SIK) as our key.
 *
 * param session [in/out].
 *
 * returns 0 on success
 *         1 on failure
 */
int lanplus_generate_k1(struct ipmi_session * session)
{
	uint32_t mac_length;
	int unsupported = 0;
	uint8_t CONST_1[36] = /*EVP_MAX_MD_SIZE = 36*/
		{0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
		 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01};

	if (session->v2_data.auth_alg == IPMI_AUTH_RAKP_NONE)
		memcpy(session->v2_data.k1, CONST_1, 20);
	else
	{
		lanplus_HMAC(session->v2_data.auth_alg,
				 session->v2_data.sik,
				 session->v2_data.sik_len,/* SIK length */
				 CONST_1,
				 IPMI_AUTHCODE_BUFFER_SIZE, /*=20*/
				 session->v2_data.k1,
				 &mac_length);
		switch(session->v2_data.auth_alg)
		{
		case IPMI_AUTH_RAKP_HMAC_SHA1  : 
			if (mac_length != SHA_DIGEST_LENGTH) unsupported = 1; 
			break;
		case IPMI_AUTH_RAKP_HMAC_MD5   : 
			if (mac_length != MD5_DIGEST_LENGTH) unsupported = 1; 
			break;
		case IPMI_AUTH_RAKP_HMAC_SHA256: 
			if (mac_length != SHA256_DIGEST_LENGTH) unsupported = 1; 
			break;
		default                        : unsupported = 1; break;
		}
		if (unsupported) {  /*was assert*/
			printf("Unsupported k1 macLength %d for auth %d\n",
				mac_length, session->v2_data.auth_alg);
			return 1;
		}
		session->v2_data.k1_len = (uint8_t)mac_length;
	}

	if (verbose >= 2)
		printbuf(session->v2_data.k1, session->v2_data.k1_len, "Generated K1");

	return 0;
}



/*
 * lanplus_generate_k2
 *
 * Generate K2, the key used for RMCP+ AES encryption.
 *
 * We use the authentication algorithm to generated the HMAC, using
 * the session integrity key (SIK) as our key.
 *
 * param session [in/out].
 *
 * returns 0 on success
 *         1 on failure
 */
int lanplus_generate_k2(struct ipmi_session * session)
{
	uint32_t mac_length;
	int unsupported = 0;
	uint8_t CONST_2[36] = /*EVP_MAX_MD_SIZE = 36*/
		{0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02,
		 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02};

	if (session->v2_data.auth_alg == IPMI_AUTH_RAKP_NONE)
		memcpy(session->v2_data.k2, CONST_2, 20);
	else
	{
		lanplus_HMAC(session->v2_data.auth_alg,
				 session->v2_data.sik,
				 session->v2_data.sik_len,/* SIK length */
				 CONST_2,
				 IPMI_AUTHCODE_BUFFER_SIZE, /*=20*/
				 session->v2_data.k2,
				 &mac_length);
		switch(session->v2_data.auth_alg)
		{
		case IPMI_AUTH_RAKP_HMAC_SHA1  : 
			if (mac_length != SHA_DIGEST_LENGTH) unsupported = 1; 
			break;
		case IPMI_AUTH_RAKP_HMAC_MD5   : 
			if (mac_length != MD5_DIGEST_LENGTH) unsupported = 1; 
			break;
		case IPMI_AUTH_RAKP_HMAC_SHA256: 
			if (mac_length != SHA256_DIGEST_LENGTH) unsupported = 1; 
			break;
		default                        : unsupported = 1; break;
		}
		if (unsupported) {  /*was assert*/
			printf("Unsupported k2 macLength %d for auth %d\n",
				mac_length, session->v2_data.auth_alg);
			return 1;
		}
		session->v2_data.k2_len = (uint8_t)mac_length;
	}

	if (verbose >= 2)
		printbuf(session->v2_data.k2, session->v2_data.k2_len, "Generated K2");

	return 0;
}



/*
 * lanplus_encrypt_payload
 *
 * Perform the appropriate encryption on the input data.  Output the encrypted
 * data to output, including the required confidentiality header and trailer.
 * If the crypt_alg is IPMI_CRYPT_NONE, simply copy the input to the output and
 * set bytes_written to input_length.
 * 
 * param crypt_alg specifies the encryption algorithm (from table 13-19 of the
 *       IPMI v2 spec)
 * param key is the used as input to the encryption algorithmf
 * param input is the input data to be encrypted
 * param input_length is the length of the input data to be encrypted
 * param output is the cipher text generated by the encryption process
 * param bytes_written is the number of bytes written during the encryption
 *       process
 *
 * returns 0 on success
 *         1 on failure
 */
int lanplus_encrypt_payload(uint8_t         crypt_alg,
							const uint8_t * key,
							const uint8_t * input,
							uint32_t          input_length,
							uint8_t       * output,
							uint16_t      * bytes_written)
{
	uint8_t * padded_input;
	uint32_t    mod, i, bytes_encrypted;
	uint8_t   pad_length = 0;

	if (crypt_alg == IPMI_CRYPT_NONE)
	{
		/* Just copy the input to the output */
		*bytes_written = (uint16_t)input_length;
		return 0;
	}
	
	/* Currently, we only support AES  (was assert) */
	if ((crypt_alg != IPMI_CRYPT_AES_CBC_128) || 
	    (input_length > IPMI_MAX_PAYLOAD_SIZE)) {
		lprintf(LOG_ERR,"lanplus crypt: unsupported alg %d or len %d\n",
			crypt_alg,input_length);
		return 1;
	}

	/*
	 * The input to the AES encryption algorithm has to be a multiple of the
	 * block size (16 bytes).  The extra byte we are adding is the pad length
	 * byte.
	 */
	mod = (input_length + 1) % IPMI_CRYPT_AES_CBC_128_BLOCK_SIZE;
	if (mod)
		pad_length = IPMI_CRYPT_AES_CBC_128_BLOCK_SIZE - mod;

	padded_input = (uint8_t*)malloc(input_length + pad_length + 1);
	if (padded_input == NULL) {
		lprintf(LOG_ERR, "lanplus: malloc failure");
		return 1;
	}
	memcpy(padded_input, input, input_length);

	/* add the pad */
	for (i = 0; i < pad_length; ++i)
		padded_input[input_length + i] = i + 1;

	/* add the pad length */
	padded_input[input_length + pad_length] = pad_length;

	/* Generate an initialization vector, IV, for the encryption process */
	if (lanplus_rand(output, IPMI_CRYPT_AES_CBC_128_BLOCK_SIZE))
	{
		lprintf(LOG_ERR, "lanplus_encrypt_payload: Error generating IV");
		free(padded_input);
		return 1;
	}

	if (verbose > 2)
		printbuf(output, IPMI_CRYPT_AES_CBC_128_BLOCK_SIZE, ">> Initialization vector");



	lanplus_encrypt_aes_cbc_128(output,                                     /* IV              */
								key,                                        /* K2              */
								padded_input,                               /* Data to encrypt */
								input_length + pad_length + 1,              /* Input length    */
								output + IPMI_CRYPT_AES_CBC_128_BLOCK_SIZE, /* output          */
								&bytes_encrypted);                          /* bytes written   */

	*bytes_written =
		IPMI_CRYPT_AES_CBC_128_BLOCK_SIZE + /* IV */
		bytes_encrypted;

	free(padded_input);

	return 0;
}



/*
 * lanplus_has_valid_auth_code
 *
 * Determine whether the packets authcode field is valid for packet.
 * 
 * We always return success if any of the following are true. 
 *  - this is not an IPMIv2 packet
 *  - the session is not yet active
 *  - the packet specifies that it is not authenticated
 *  - the integrity algorithm agreed upon during session creation is "none"
 *
 * The authcode is computed using the specified integrity algorithm starting
 * with the AuthType / Format field, and ending with the field immediately
 * preceeding the authcode itself.
 *
 * The key key used to generate the authcode MAC is K1.
 * 
 * param rs holds the response structure.
 * param session holds our session state, including our chosen algorithm, key, etc.
 *
 * returns 1 on success (authcode is valid)
 *         0 on failure (autchode integrity check failed)
 */
int lanplus_has_valid_auth_code(struct ipmi_rs * rs,
					struct ipmi_session * session)
{
	uint8_t * bmc_authcode;
	uint8_t   generated_authcode[EVP_MAX_MD_SIZE];
	uint32_t    generated_authcode_length;
	uint32_t    authcode_length;
	

	if ((rs->session.authtype != IPMI_SESSION_AUTHTYPE_RMCP_PLUS) ||
		(session->v2_data.session_state != LANPLUS_STATE_ACTIVE)  ||
		(! rs->session.bAuthenticated)                            ||
		(session->v2_data.integrity_alg == IPMI_INTEGRITY_NONE))
		return 1;
	
	switch(session->v2_data.integrity_alg)
	{
	case IPMI_INTEGRITY_HMAC_SHA1_96   : authcode_length = IPMI_SHA1_AUTHCODE_SIZE; break;
	case IPMI_INTEGRITY_HMAC_MD5_128   : authcode_length = IPMI_HMAC_MD5_AUTHCODE_SIZE; break;
	case IPMI_INTEGRITY_HMAC_SHA256_128: authcode_length = IPMI_HMAC_SHA256_AUTHCODE_SIZE; break;
	/* Unsupported */
	default: printf("Unsupported lanplus auth_code %d\n",
				 session->v2_data.auth_alg);
		return 1; break;
	}

	/*
	 * For SHA1-96, the authcode will be the last 12 bytes in the packet
	 */
	bmc_authcode = rs->data + (rs->data_len - authcode_length);

	lanplus_HMAC(session->v2_data.integrity_alg,
				 session->v2_data.k1,
				 session->v2_data.k1_len,
				 rs->data + IPMI_LANPLUS_OFFSET_AUTHTYPE,
				 rs->data_len - IPMI_LANPLUS_OFFSET_AUTHTYPE - authcode_length,
				 generated_authcode,
				 &generated_authcode_length);

	if (verbose > 3)
	{
		lprintf(LOG_DEBUG+2, "Validating authcode");
		printbuf(session->v2_data.k1, session->v2_data.k1_len, "K1");
		printbuf(rs->data + IPMI_LANPLUS_OFFSET_AUTHTYPE,
				 rs->data_len - IPMI_LANPLUS_OFFSET_AUTHTYPE - authcode_length,
				 "Authcode Input Data");
		printbuf(generated_authcode, authcode_length, "Generated authcode");
		printbuf(bmc_authcode,       authcode_length, "Expected authcode");
	}

//	assert(generated_authcode_length == 20);
	return (memcmp(bmc_authcode, generated_authcode, authcode_length) == 0);
}



/*
 * lanplus_decrypt_payload
 *
 * 
 * param input points to the beginning of the payload (which will be the IV if
 *       we are using AES)
 * param payload_size [out] will be set to the size of the payload EXCLUDING
 * padding
 * 
 * returns 0 on success (we were able to successfully decrypt the packet)
 *         1 on failure (we were unable to successfully decrypt the packet)
 */
int lanplus_decrypt_payload(uint8_t         crypt_alg,
					const uint8_t * key,
					const uint8_t * input,
					uint32_t        input_length,
					uint8_t       * output,
					uint16_t      * payload_size)
{
	uint8_t * decrypted_payload;
	uint32_t    bytes_decrypted;

	if (crypt_alg == IPMI_CRYPT_NONE)
	{
		/* We are not encrypted.  The paylaod size is is everything. */
		*payload_size = (uint16_t)input_length;
		memmove(output, input, input_length);
		return 0;
	}

	/* We only support AES (was assert) */
	if (crypt_alg != IPMI_CRYPT_AES_CBC_128) {
		lprintf(LOG_ERR,"lanplus decrypt: unsupported alg %d\n",
			crypt_alg);
		return 1;
	}

	decrypted_payload = (uint8_t*)malloc(input_length);
	if (decrypted_payload == NULL) {
		lprintf(LOG_ERR, "lanplus: malloc failure");
		return 1;
	}

	lanplus_decrypt_aes_cbc_128(input,                /* IV              */
				key,                      /* Key             */
				input             +       /* Data to decrypt */
				IPMI_CRYPT_AES_CBC_128_BLOCK_SIZE,
				input_length -            /* Input length    */
				IPMI_CRYPT_AES_CBC_128_BLOCK_SIZE,
				decrypted_payload,        /* output          */
				&bytes_decrypted);        /* bytes written   */

	if (bytes_decrypted != 0)
	{
		/* Success */
		uint8_t conf_pad_length;
		int i;

		memmove(output,
				decrypted_payload,
				bytes_decrypted);

		/*
		 * We have to determine the payload size, by substracting the padding, etc.
		 * The last byte of the decrypted payload is the confidentiality pad length.
		 */
		conf_pad_length = decrypted_payload[bytes_decrypted - 1];
		*payload_size = bytes_decrypted - conf_pad_length - 1;

		/*
		 * Extra test to make sure that the padding looks like it should (should start
		 * with 0x01, 0x02, 0x03, etc...
		 */
		for (i = 0; i < conf_pad_length; ++i)
		{
			if (decrypted_payload[*payload_size + i] == i)
			{
				lprintf(LOG_ERR, "Malformed payload padding");
				return 1; /* assert(0); */
			}
		}
	}
	else
	{
		lprintf(LOG_ERR, "ERROR: lanplus_decrypt_aes_cbc_128 decryptd 0 bytes");
		return 1; /* assert(0); */
	}

	free(decrypted_payload);
	return (bytes_decrypted == 0);
}
