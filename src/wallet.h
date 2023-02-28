// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2012 The Bitcoin developers
// Copyright (c) 2014 supercoindev
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef BITCOIN_WALLET_H
#define BITCOIN_WALLET_H

#include <string>
#include <vector>

#include <stdlib.h>

#include "main.h"
#include "key.h"
#include "keystore.h"
#include "script.h"
#include "ui_interface.h"
#include "util.h"
#include "walletdb.h"

extern bool fWalletUnlockStakingOnly;
extern bool fConfChange;
class CAccountingEntry;
class CWalletTx;
class CReserveKey;
class COutput;
class CCoinControl;

/** (client) version numbers for particular wallet features */
enum WalletFeature
{
    FEATURE_BASE = 10500, // the earliest version new wallets supports (only useful for getinfo's clientversion output)

    FEATURE_WALLETCRYPT = 40000, // wallet encryption
    FEATURE_COMPRPUBKEY = 60000, // compressed public keys

    FEATURE_LATEST = 60000
};

enum AnonymousTxRole
{
	ROLE_UNKNOWN	= 0,
	ROLE_SENDER		= 1,
	ROLE_MIXER		= 2,
	ROLE_GUARANTOR	= 3
};

enum AnonymousTxStatus
{
	ATX_STATUS_NONE		= 0,
	ATX_STATUS_RESERVE	= 1,
	ATX_STATUS_INITDATA	= 2,
	ATX_STATUS_PUBKEY	= 3,
	ATX_STATUS_MSADDR	= 4,
	ATX_STATUS_MSDEPO	= 5,
	ATX_STATUS_MSDEPV	= 6,
	ATX_STATUS_MSTXR0	= 7,
	ATX_STATUS_MSTXR1	= 8,
	ATX_STATUS_MSTXRC	= 9,
	ATX_STATUS_COMPLETE = 10
};

/** A key pool entry */
class CKeyPool
{
public:
    int64_t nTime;
    CPubKey vchPubKey;

    CKeyPool()
    {
        nTime = GetTime();
    }

    CKeyPool(const CPubKey& vchPubKeyIn)
    {
        nTime = GetTime();
        vchPubKey = vchPubKeyIn;
    }

    IMPLEMENT_SERIALIZE
    (
        if (!(nType & SER_GETHASH))
            READWRITE(nVersion);
        READWRITE(nTime);
        READWRITE(vchPubKey);
    )
};

class MultisigTxInfo
{
private:
	std::string		tx;
	int				signedCount;
	std::string		txidSender;
	std::string		txidMixer;	
	std::string		txidGuarantor;
	int				voutNSender;
	int				voutNMixer;
	int				voutNGuarantor;
	std::string		sPubKeySender;
	std::string		sPubKeyMixer;	
	std::string		sPubKeyGuarantor;

public:
	MultisigTxInfo()
	{
		tx = "";
		signedCount = 0;
		txidSender = "";
		txidMixer = "";
		txidGuarantor = "";
		voutNSender = 0;
		voutNMixer = 0;
		voutNGuarantor = 0;
		sPubKeySender = "";
		sPubKeyMixer = "";
		sPubKeyGuarantor = "";
	}

	void clean()
	{
		tx = "";
		signedCount = 0;
		txidSender = "";
		txidMixer = "";
		txidGuarantor = "";
		voutNSender = 0;
		voutNMixer = 0;
		voutNGuarantor = 0;
		sPubKeySender = "";
		sPubKeyMixer = "";
		sPubKeyGuarantor = "";
	}

	std::string GetTx() const
	{
		return tx;
	}

	int GetSignedCount() const
	{
		return signedCount;
	}

	std::string GetTxid(AnonymousTxRole role) const
	{
		std::string txid = "";

		switch (role)
		{
			case ROLE_SENDER:
				txid = txidSender;
				break;

			case ROLE_MIXER:
				txid = txidMixer;
				break;

			case ROLE_GUARANTOR:
				txid = txidGuarantor;
				break;
		}

		return txid;
	}

	void GetTxOutInfo(AnonymousTxRole role, std::string& txid, int& voutn, std::string& pubkey) const
	{
		txid = "";
		voutn = 0;
		pubkey = "";

		switch (role)
		{
			case ROLE_SENDER:
				txid = txidSender;
				voutn = voutNSender;
				pubkey = sPubKeySender;
				break;

			case ROLE_MIXER:
				txid = txidMixer;
				voutn = voutNMixer;
				pubkey = sPubKeyMixer;
				break;

			case ROLE_GUARANTOR:
				txid = txidGuarantor;
				voutn = voutNGuarantor;
				pubkey = sPubKeyGuarantor;
				break;
		}
	}

	void SetTxid(AnonymousTxRole role, std::string txid)
	{
		switch (role)
		{
			case ROLE_SENDER:
				txidSender = txid;
				break;

			case ROLE_MIXER:
				txidMixer = txid;
				break;

			case ROLE_GUARANTOR:
				txidGuarantor = txid;
				break;
		}
	}

	void SetVoutAndScriptPubKey(AnonymousTxRole role, int voutn, std::string scriptPubKey)
	{
		switch (role)
		{
			case ROLE_SENDER:
				voutNSender = voutn;
				sPubKeySender = scriptPubKey;
				break;

			case ROLE_MIXER:
				voutNMixer = voutn;
				sPubKeyMixer = scriptPubKey;
				break;

			case ROLE_GUARANTOR:
				voutNGuarantor = voutn;
				sPubKeyGuarantor = scriptPubKey;
				break;
		}
	}

	void SetTx(std::string tx0, int scount)
	{
		tx = tx0;
		signedCount = scount;
	}

	bool IsTxidComplete() const
	{
		bool b = (txidSender != "") && (txidMixer != "") && (txidGuarantor != "");
		return b;
	}
};

class AnonymousTxParties
{
private:
	AnonymousTxRole	role;
	CNode*	pSender;
	CNode*	pMixer;
	CNode*	pGuarantor;
	std::string	addressSender;
	std::string	addressMixer;
	std::string	addressGuarantor;
	std::string	pubKeySender;
	std::string	pubKeyMixer;
	std::string	pubKeyGuarantor;

public:
	AnonymousTxParties()
	{
		pSender = NULL;
		pMixer = NULL;
		pGuarantor = NULL;
		role = ROLE_UNKNOWN;
		addressSender = "";
		addressMixer = "";
		addressGuarantor = "";
		pubKeySender = "";
		pubKeyMixer = "";
		pubKeyGuarantor = "";
	}

	AnonymousTxRole GetRole() const
	{
		return role;
	}

	std::string GetSelfAddress() const
	{
		std::string address = "";

		switch (role)
		{
			case ROLE_SENDER:
				address = addressSende