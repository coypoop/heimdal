/*
 * Copyright (c) 1997 Kungliga Tekniska H�gskolan
 * (Royal Institute of Technology, Stockholm, Sweden). 
 * All rights reserved. 
 *
 * Redistribution and use in source and binary forms, with or without 
 * modification, are permitted provided that the following conditions 
 * are met: 
 *
 * 1. Redistributions of source code must retain the above copyright 
 *    notice, this list of conditions and the following disclaimer. 
 *
 * 2. Redistributions in binary form must reproduce the above copyright 
 *    notice, this list of conditions and the following disclaimer in the 
 *    documentation and/or other materials provided with the distribution. 
 *
 * 3. All advertising materials mentioning features or use of this software 
 *    must display the following acknowledgement: 
 *      This product includes software developed by Kungliga Tekniska 
 *      H�gskolan and its contributors. 
 *
 * 4. Neither the name of the Institute nor the names of its contributors 
 *    may be used to endorse or promote products derived from this software 
 *    without specific prior written permission. 
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND 
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE 
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE 
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL 
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS 
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) 
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT 
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY 
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF 
 * SUCH DAMAGE. 
 */

#include <krb5_locl.h>

RCSID("$Id$");

krb5_error_code
krb5_build_authenticator (krb5_context context,
			  krb5_auth_context auth_context,
			  krb5_creds *cred,
			  Checksum *cksum,
			  Authenticator **auth_result,
			  krb5_data *result)
{
  Authenticator *auth;
  unsigned char buf[1024];
  size_t len;
  krb5_error_code ret;
  krb5_enctype enctype;

  if (auth_context->enctype)
      enctype = auth_context->enctype;
  else {
      ret = krb5_keytype_to_etype(context,
				  cred->session.keytype,
				  &enctype);
      if (ret)
	  return ret;
  }

  auth = malloc(sizeof(*auth));
  if (auth == NULL)
      return ENOMEM;

  auth->authenticator_vno = 5;
  copy_Realm(&cred->client->realm, &auth->crealm);
  copy_PrincipalName(&cred->client->name, &auth->cname);

  {
      int32_t sec, usec;

      krb5_us_timeofday (context, &sec, &usec);
      auth->ctime = sec;
      auth->cusec = usec;
  }
#if 0
  auth->subkey = NULL;
#else
  krb5_generate_subkey (context, &cred->session, &auth->subkey);
  free_EncryptionKey (&auth_context->local_subkey);
  copy_EncryptionKey (auth->subkey,
		      &auth_context->local_subkey);
#endif
  if (auth_context->flags & KRB5_AUTH_CONTEXT_DO_SEQUENCE) {
    krb5_generate_seq_number (context,
			      &cred->session, 
			      &auth_context->local_seqnumber);
    ALLOC(auth->seq_number, 1);
    *auth->seq_number = auth_context->local_seqnumber;
  } else
    auth->seq_number = NULL;
  auth->authorization_data = NULL;
  auth->cksum = cksum;

  /* XXX - Copy more to auth_context? */

  if (auth_context) {
    auth_context->authenticator->ctime = auth->ctime;
    auth_context->authenticator->cusec = auth->cusec;
  }

  memset (buf, 0, sizeof(buf));
  ret = encode_Authenticator (buf + sizeof(buf) - 1, sizeof(buf), auth, &len);

  ret = krb5_encrypt (context, buf + sizeof(buf) - len, len,
		      enctype,
		      &cred->session,
		      result);

  if (auth_result)
    *auth_result = auth;
  else {
    /* Don't free the `cksum', it's allocated by the caller */
    auth->cksum = NULL;
    free_Authenticator (auth);
    free (auth);
  }
  return ret;
}
