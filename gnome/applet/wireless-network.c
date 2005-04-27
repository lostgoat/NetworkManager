/* -*- Mode: C; tab-width: 5; indent-tabs-mode: t; c-basic-offset: 5 -*- */
/* NetworkManager Wireless Applet -- Display wireless access points and allow user control
 *
 * Dan Williams <dcbw@redhat.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * (C) Copyright 2004-2005 Red Hat, Inc.
 */


#include <glib.h>
#include "wireless-network.h"

/*
 * Representation of a wireless network
 *
 */
struct WirelessNetwork
{
	int		refcount;
	char *	nm_path;
	char *	essid;
	gboolean	encrypted;
	gboolean	active;
	gint8	strength;
};


/*
 * wireless_network_new
 *
 * Create a new wireless network structure
 *
 */
WirelessNetwork *wireless_network_new (const char *essid, const char *nm_path)
{
	WirelessNetwork *net = NULL;

	g_return_val_if_fail (essid != NULL, NULL);
	g_return_val_if_fail (nm_path != NULL, NULL);

	if ((net = g_malloc0 (sizeof (WirelessNetwork))))
	{
		net->essid = g_strdup (essid);
		net->nm_path = g_strdup (nm_path);
	}

	return (net);
}


/*
 * wireless_network_copy
 *
 * Create a new wireless network structure from an existing one
 *
 */
WirelessNetwork *wireless_network_copy (WirelessNetwork *src)
{
	WirelessNetwork *net = NULL;

	g_return_val_if_fail (src != NULL, NULL);

	if ((net = g_malloc0 (sizeof (WirelessNetwork))))
	{
		net->refcount = 1;
		net->nm_path = g_strdup (src->nm_path);
		net->essid = g_strdup (src->essid);
		net->active = src->active;
		net->encrypted = src->encrypted;
		net->strength = src->strength;
	}

	return (net);
}


/*
 * wireless_network_ref
 *
 * Increment the reference count of the wireless network
 *
 */
void wireless_network_ref (WirelessNetwork *net)
{
	g_return_if_fail (net != NULL);

	net->refcount++;
}


/*
 * wireless_network_unref
 *
 * Unrefs (and possibly frees) the representation of a wireless network
 *
 */
void wireless_network_unref (WirelessNetwork *net)
{
	g_return_if_fail (net != NULL);

	net->refcount--;
	if (net->refcount < 1)
	{
		g_free (net->nm_path);
		g_free (net->essid);
		g_free (net);
	}
}


/*
 * Accessors for active
 */
gboolean wireless_network_get_active (WirelessNetwork *net)
{
	g_return_val_if_fail (net != NULL, FALSE);

	return net->active;
}

void wireless_network_set_active (WirelessNetwork *net, gboolean active)
{
	g_return_if_fail (net != NULL);

	net->active = active;
}

/*
 * Accessors for essid
 */
const char *wireless_network_get_essid (WirelessNetwork *net)
{
	g_return_val_if_fail (net != NULL, FALSE);

	return net->essid;
}

/*
 * Accessors for nm_path
 */
const char *wireless_network_get_nm_path (WirelessNetwork *net)
{
	g_return_val_if_fail (net != NULL, FALSE);

	return net->nm_path;
}

/*
 * Accessors for encrypted
 */
gboolean wireless_network_get_encrypted (WirelessNetwork *net)
{
	g_return_val_if_fail (net != NULL, FALSE);

	return net->encrypted;
}

void wireless_network_set_encrypted (WirelessNetwork *net, gboolean encrypted)
{
	g_return_if_fail (net != NULL);

	net->encrypted = encrypted;
}

/*
 * Accessors for strength
 */
gint8 wireless_network_get_strength (WirelessNetwork *net)
{
	g_return_val_if_fail (net != NULL, FALSE);

	return net->strength;
}

void wireless_network_set_strength (WirelessNetwork *net, gint8 strength)
{
	g_return_if_fail (net != NULL);

	net->strength = strength;
}

