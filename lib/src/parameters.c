/*********************************************************************
 *                
 * Filename:      parameters.c
 * Version:       1.0
 * Description:   A more general way to handle (pi,pl,pv) parameters
 * Status:        Experimental.
 * Author:        Dag Brattli <dagb@cs.uit.no>
 * Created at:    Mon Jun  7 10:25:11 1999
 * Modified at:   Mon Oct 18 16:11:32 1999
 * Modified by:   Dag Brattli <dagb@cs.uit.no>
 * 
 *     Copyright (c) 1999 Dag Brattli, All Rights Reserved.
 *     
 *     This program is free software; you can redistribute it and/or 
 *     modify it under the terms of the GNU General Public License as 
 *     published by the Free Software Foundation; either version 2 of 
 *     the License, or (at your option) any later version.
 * 
 *     This program is distributed in the hope that it will be useful,
 *     but WITHOUT ANY WARRANTY; without even the implied warranty of
 *     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *     GNU General Public License for more details.
 * 
 *     You should have received a copy of the GNU General Public License 
 *     along with this program; if not, write to the Free Software 
 *     Foundation, Inc., 59 Temple Place, Suite 330, Boston, 
 *     MA 02111-1307 USA
 *     
 ********************************************************************/

#include <string.h>

#include <obex_main.h>
#include <parameters.h>

#define put_unaligned(a,b) (*(b) = a)
#define get_unaligned(a) *(a)

static gint irda_extract_integer(void *self, guint8 *buf, gint len, guint8 pi, 
				 PV_TYPE type, PI_HANDLER func);
static gint irda_extract_string(void *self, guint8 *buf, gint len, guint8 pi, 
			       PV_TYPE type, PI_HANDLER func);
static gint irda_extract_octseq(void *self, guint8 *buf, gint len, guint8 pi, 
				PV_TYPE type, PI_HANDLER func);
static gint irda_extract_no_value(void *self, guint8 *buf, gint len, guint8 pi,
				  PV_TYPE type, PI_HANDLER func);

static gint irda_insert_integer(void *self, guint8 *buf, gint len, guint8 pi, 
				PV_TYPE type, PI_HANDLER func);
static gint irda_insert_no_value(void *self, guint8 *buf, gint len, guint8 pi, 
				 PV_TYPE type, PI_HANDLER func);

/* Parameter value call table. Must match PV_TYPE */
static PV_HANDLER pv_extract_table[] = {
	irda_extract_integer, /* Handler for any length integers */
	irda_extract_integer, /* Handler for 8  bits integers */
	irda_extract_integer, /* Handler for 16 bits integers */
	irda_extract_string,  /* Handler for strings */
	irda_extract_integer, /* Handler for 32 bits integers */
	irda_extract_octseq,  /* Handler for octet sequences */
	irda_extract_no_value /* Handler for no value parameters */
};

static PV_HANDLER pv_insert_table[] = {
	irda_insert_integer, /* Handler for any length integers */
	irda_insert_integer, /* Handler for 8  bits integers */
	irda_insert_integer, /* Handler for 16 bits integers */
	NULL,                /* Handler for strings */
	irda_insert_integer, /* Handler for 32 bits integers */
	NULL,                /* Handler for octet sequences */
	irda_insert_no_value /* Handler for no value parameters */
};

/*
 * Function irda_insert_no_value (self, buf, len, pi, type, func)
 *
 *    
 *
 */
static gint irda_insert_no_value(void *self, guint8 *buf, gint len, guint8 pi, 
				 PV_TYPE type, PI_HANDLER func)
{
	param_t p;
	gint ret;

	p.pi = pi;
	p.pl = 0;

	/* Call handler for this parameter */
	ret = (*func)(self, &p, PV_GET);

	/* Extract values anyway, since handler may need them */
	irda_param_pack(buf, "bb", p.pi, &p.pl);

	if (ret < 0)
		return ret;
	 
	return 2; /* Inserted pl+2 bytes */
}

/*
 * Function irda_extract_no_value (self, buf, len, type, func)
 *
 *    Extracts a parameter without a pv field (pl=0)
 *
 */
static gint irda_extract_no_value(void *self, guint8 *buf, gint len, guint8 pi,
				  PV_TYPE type, PI_HANDLER func)
{
	param_t p;
	gint ret;

	/* Extract values anyway, since handler may need them */
	irda_param_unpack(buf, "bb", &p.pi, &p.pl);

	/* Call handler for this parameter */
	ret = (*func)(self, &p, PV_PUT);

	if (ret < 0)
		return ret;
	 
	return 2; /* Extracted pl+2 bytes */
}

/*
 * Function irda_insert_integer (self, buf, len, pi, type, func)
 *
 *    
 *
 */
static gint irda_insert_integer(void *self, guint8 *buf, gint len, guint8 pi, 
				PV_TYPE type, PI_HANDLER func)
{
	param_t p;
	gint n = 0;
	gint err;

	p.pi = pi;             /* In case handler needs to know */
	p.pl = type & PV_MASK; /* The integer type codes the lenght as well */
	p.pv.i = 0;            /* Clear value */

	/* Call handler for this parameter */
	err = (*func)(self, &p, PV_GET);
	if (err < 0)
		return err; 

	/* 
	 * If parameter lenght is still 0, then (1) this is an any length 
	 * integer, and (2) the handler function does not care which length
	 * we choose to use, so we pick the one the gives the fewest bytes.
	 */
	if (p.pl == 0) {
		if (p.pv.i < 0xff) {
			g_print(__FUNCTION__ "(), using 1 byte\n");
			p.pl = 1;
		} else if (p.pv.i < 0xffff) {
			g_print(__FUNCTION__ "(), using 2 bytes\n");
			p.pl = 2;
		} else {
			g_print(__FUNCTION__ "(), using 4 bytes\n");
			p.pl = 4; /* Default length */
		}
	}
	/* Check if buffer is long enough for insertion */
	if (len < (2+p.pl)) {
		g_print(__FUNCTION__ "(), buffer to short for insertion!\n");
		return -1;
	}
	DEBUG(2, __FUNCTION__ "(), pi=%#x, pl=%d, pi=%d\n", p.pi, p.pl, 
	      p.pv.i);
	switch (p.pl) {
	case 1:
		n += irda_param_pack(buf, "bbb", p.pi, p.pl, p.pv.b);
		break;
	case 2:
		if (type & PV_BIG_ENDIAN)
			p.pv.s = GUINT16_TO_BE(p.pv.s);
		else
			p.pv.s = GUINT16_TO_LE(p.pv.s);
		n += irda_param_pack(buf, "bbs", p.pi, p.pl, p.pv.s);
		break;
	case 4:
		if (type & PV_BIG_ENDIAN)
			p.pv.i = GUINT32_TO_BE(p.pv.i);
		else
			p.pv.i = GUINT32_TO_LE(p.pv.i);
		n += irda_param_pack(buf, "bbi", p.pi, p.pl, p.pv.i);
		
		break;
	default:
		g_print(__FUNCTION__ "() length %d not supported\n", p.pl);
		/* Skip parameter */ 
		return -1;
	}
	
	return p.pl+2; /* Inserted pl+2 bytes */
}

/*
 * Function irda_extract integer (self, buf, len, pi, type, func)
 *
 *    Extract a possibly variable length integer from buffer, and call 
 *    handler for processing of the parameter
 */
static gint irda_extract_integer(void *self, guint8 *buf, gint len, guint8 pi, 
				 PV_TYPE type, PI_HANDLER func)
{
	param_t p;
	gint n = 0;
	gint err;

	p.pi = pi;     /* In case handler needs to know */
	p.pl = buf[1]; /* Extract lenght of value */
	p.pv.i = 0;    /* Clear value */

	/* Check if buffer is long enough for parsing */
	if (len < (2+p.pl)) {
		g_print(__FUNCTION__ "(), buffer to short for parsing! "
			"Need %d bytes, but len is only %d\n", p.pl, len);
		return -1;
	}

	/* 
	 * Check that the integer length is what we expect it to be. If the
	 * handler want a 16 bits integer then a 32 bits is not good enough
	 */
	if (((type & PV_MASK) != PV_INTEGER) && ((type & PV_MASK) != p.pl)) {
		g_print(__FUNCTION__ "(), invalid parameter length! "
		      "Expected %d bytes, but value had %d bytes!\n",
		      type & PV_MASK, p.pl);
		
		/* Skip parameter */
		return p.pl+2;
	}

	switch (p.pl) {
	case 1:
		n += irda_param_unpack(buf+2, "b", &p.pv.b);
		break;
	case 2:
		n += irda_param_unpack(buf+2, "s", &p.pv.s);
		if (type & PV_BIG_ENDIAN)
			p.pv.s = GUINT16_FROM_BE(p.pv.s);
		else
			p.pv.s = GUINT16_FROM_LE(p.pv.s);
		break;
	case 4:
		n += irda_param_unpack(buf+2, "i", &p.pv.i);
		if (type & PV_BIG_ENDIAN)
			p.pv.i = GUINT32_FROM_BE(p.pv.i);
		else
			p.pv.i = GUINT32_FROM_LE(p.pv.i);
		break;
	default:
		g_print(__FUNCTION__ "() length %d not supported\n", p.pl);
		
		/* Skip parameter */ 
		return p.pl+2;
	}
	
	/* Call handler for this parameter */
	err = (*func)(self, &p, PV_PUT);
	if (err < 0)
		return err; 
	
	return p.pl+2; /* Extracted pl+2 bytes */
}

/*
 * Function irda_extract_string (self, buf, len, type, func)
 *
 *    
 *
 */
static gint irda_extract_string(void *self, guint8 *buf, gint len, guint8 pi, 
				PV_TYPE type, PI_HANDLER func)
{
	gchar str[33];
	param_t p;
	gint err;

	g_print(__FUNCTION__ "()\n");

	p.pi = pi;     /* In case handler needs to know */
	p.pl = buf[1]; /* Extract lenght of value */

	DEBUG(2, __FUNCTION__ "(), pi=%#x, pl=%d\n", p.pi, p.pl);

	/* Check if buffer is long enough for parsing */
	if (len < (2+p.pl)) {
		g_print(__FUNCTION__ "(), buffer to short for parsing! "
			"Need %d bytes, but len is only %d\n", p.pl, len);
		return -1;
	}
	
	/* Should be safe to copy string like this since we have already 
	 * checked that the buffer is long enough */
	strncpy(str, buf+2, p.pl);
	
	DEBUG(2, __FUNCTION__ "(), str=0x%02x 0x%02x\n", (guint8) str[0], 
	      (guint8) str[1]);
	
	/* Null terminate string */
	str[p.pl+1] = '\0';

	p.pv.c = str; /* Handler will need to take a copy */
	
	/* Call handler for this parameter */
	err = (*func)(self, &p, PV_PUT);
	if (err < 0)
		return err; 
	
	return p.pl+2; /* Extracted pl+2 bytes */
}

/*
 * Function irda_extract_octseq (self, buf, len, type, func)
 *
 *    
 *
 */
static gint irda_extract_octseq(void *self, guint8 *buf, gint len, guint8 pi,
				PV_TYPE type, PI_HANDLER func)
{
	param_t p;

	p.pi = pi;     /* In case handler needs to know */
	p.pl = buf[1]; /* Extract lenght of value */
	
	/* Check if buffer is long enough for parsing */
	if (len < (2+p.pl)) {
		g_print(__FUNCTION__ "(), buffer to short for parsing! "
			"Need %d bytes, but len is only %d\n", p.pl, len);
		return -1;
	}
	
	DEBUG(0, __FUNCTION__ "(), not impl\n");
	
	return p.pl+2; /* Extracted pl+2 bytes */
}

/*
 * Function irda_param_pack (skb, fmt, ...)
 *
 *    Format:
 *        'i' = 32 bits integer
 *        's' = string
 *
 */
gint irda_param_pack(guint8 *buf, gchar *fmt, ...)
{
	va_list args;
	gchar *p;
	gint n = 0;
	pv_t arg;
	
	va_start(args, fmt);

	for (p = fmt; *p != '\0'; p++) {
		switch (*p) {
		case 'b':  /* 8 bits unsigned byte */
			buf[n++] = va_arg(args, guint8);
			break;
		case 's':  /* 16 bits unsigned short */
			arg.s = va_arg(args, guint16);
			put_unaligned(arg.s, (guint16 *)(buf+n)); n+=2;
			break;
		case 'i':  /* 32 bits unsigned integer */
			arg.i = va_arg(args, guint32);
			put_unaligned(arg.i, (guint32 *)(buf+n)); n+=4;
			break;
		case 'c': /* \0 terminated string */
			arg.c = va_arg(args, gchar *);
			strcpy(buf+n, arg.c);
			n += strlen(arg.c) + 1;
			break;
		default:
			va_end(args);
			return -1;
		}
		
	}
	va_end(args);

	return 0;
}

/*
 * Function irda_param_unpack (skb, fmt, ...)
 *
 *    
 *
 */
gint irda_param_unpack(guint8 *buf, gchar *fmt, ...)
{
	va_list args;
	gchar *p;
	gint n = 0;
	pv_t arg;

	va_start(args, fmt);

	for (p = fmt; *p != '\0'; p++) {
		switch (*p) {
		case 'b':  /* 8 bits byte */
			arg.bp = va_arg(args, guint8 *);
			*arg.bp = buf[n++];
			break;
		case 's':  /* 16 bits short */
			arg.sp = va_arg(args, guint16 *);
			*arg.sp = get_unaligned((guint16 *)(buf+n)); n+=2;
			break;
		case 'i':  /* 32 bits unsigned integer */
			arg.ip = va_arg(args, guint32 *);
			*arg.ip = get_unaligned((guint32 *)(buf+n)); n+=4;
			break;
#if 0
		case 'c':   /* \0 terminated string */
			arg.c = va_arg(args, gchar *);
			strcpy(arg.c, buf+n);
			n += strlen(arg.c) + 1;
			break;
#endif
		default:
			va_end(args);
			return -1;
		}
		
	}
	va_end(args);

	return 0;
}

/*
 * Function irda_param_insert (self, pi, buf, len, info)
 *
 *    Insert the specified parameter (pi) into buffer. Returns number of
 *    bytes inserted
 */
gint irda_param_insert(void *self, guint8 pi, guint8 *buf, gint len, 
		       pi_param_info_t *info)
{
	pi_minor_info_t *pi_minor_info;
	guint8 pi_minor;
	guint8 pi_major;
	gint type;
	gint ret = -1;
	gint n = 0;

	g_return_val_if_fail(buf != NULL, ret);
	g_return_val_if_fail(info != 0, ret);

	pi_minor = pi & info->pi_mask;
	pi_major = pi >> info->pi_major_offset;

	/* Check if the identifier value (pi) is valid */
	if ((pi_major > info->len-1) || 
	    (pi_minor > info->tables[pi_major].len-1))
	{
		DEBUG(0, __FUNCTION__ 
		      "(), no handler for parameter=0x%02x\n", pi);
		
		/* Skip this parameter */
		return -1;
	}
	
	/* Lookup the info on how to parse this parameter */
	pi_minor_info = &info->tables[pi_major].pi_minor_call_table[pi_minor];

	/* Find expected data type for this parameter identifier (pi)*/
	type = pi_minor_info->type;

	DEBUG(3, __FUNCTION__ "(), pi=[%d,%d], type=%d\n",
	      pi_major, pi_minor, type);
		
	/*  Check if handler has been implemented */
	if (!pi_minor_info->func) {
		g_print(__FUNCTION__"(), no handler for pi=%#x\n", pi);
		/* Skip this parameter */
		return -1;
	}
	
	/* Insert parameter value */
	ret = (*pv_insert_table[type & PV_MASK])(self, buf+n, len, pi, type, 
						 pi_minor_info->func);
	return ret;
}

/*
 * Function irda_param_extract_all (self, buf, len, info)
 *
 *    Parse all parameters. If len is correct, then everything should be
 *    safe. Returns the number of bytes that was parsed
 *
 */
gint irda_param_extract(void *self, guint8 *buf, gint len, 
			pi_param_info_t *info)
{
	pi_minor_info_t *pi_minor_info;
	guint8 pi_minor;
	guint8 pi_major;
	gint type;
	gint ret = -1;
	gint n = 0;

	g_return_val_if_fail(buf != NULL, ret);
	g_return_val_if_fail(info != 0, ret);

	pi_minor = buf[n] & info->pi_mask;
	pi_major = buf[n] >> info->pi_major_offset;
	
	/* Check if the identifier value (pi) is valid */
	if ((pi_major > info->len-1) || 
	    (pi_minor > info->tables[pi_major].len-1))
	{
		g_print(__FUNCTION__ "(), no handler for parameter=0x%02x\n",
			buf[0]);
		
		/* Skip this parameter */
		n += (2 + buf[n+1]);
		len -= (2 + buf[n+1]);

		return 0;  /* Continue */
	}

	/* Lookup the info on how to parse this parameter */
	pi_minor_info = &info->tables[pi_major].pi_minor_call_table[pi_minor];

	/* Find expected data type for this parameter identifier (pi)*/
	type = pi_minor_info->type;
	
	g_print(__FUNCTION__ "(), pi=[%d,%d], type=%d\n",
		pi_major, pi_minor, type);
	
	/*  Check if handler has been implemented */
	if (!pi_minor_info->func) {
		g_print(__FUNCTION__"(), no handler for pi=%#x\n", buf[n]);
		/* Skip this parameter */
		n += (2 + buf[n+1]);
		len -= (2 + buf[n+1]);
		
		return 0; /* Continue */
	}
	
	/* Parse parameter value */
	ret = (*pv_extract_table[type & PV_MASK])(self, buf+n, len, buf[n],
						  type, pi_minor_info->func);
	return ret;
}

/*
 * Function irda_param_extract_all (self, buf, len, info)
 *
 *    Parse all parameters. If len is correct, then everything should be
 *    safe. Returns the number of bytes that was parsed
 *
 */
gint irda_param_extract_all(void *self, guint8 *buf, gint len, 
			    pi_param_info_t *info)
{
	gint ret = -1;
	gint n = 0;

	g_return_val_if_fail(buf != NULL, ret);
	g_return_val_if_fail(info != 0, ret);

	/*
	 * Parse all parameters. Each parameter must be at least two bytes
	 * long or else there is no point in trying to parse it
	 */
	while (len > 2) {
		ret = irda_param_extract(self, buf+n, len, info);
		if (ret < 0)
			return ret;

		n += ret;
		len -= ret;
	}
	return n;
}

