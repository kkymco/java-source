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
				address = addressSender;
				break;

			case ROLE_MIXER:
				address = addressMixer;
				break;

			case ROLE_GUARANTOR:
				address = addressGuarantor;
				break;
		}

		return address;
	}

	std::string GetAddress(AnonymousTxRole role0) const
	{
		std::string address = "";

		switch (role0)
		{
			case ROLE_SENDER:
				address = addressSender;
				break;

			case ROLE_MIXER:
				address = addressMixer;
				break;

			case ROLE_GUARANTOR:
				address = addressGuarantor;
				break;
		}

		return address;
	}

	std::string GetSelfPubKey() const
	{
		std::string pubKey = "";

		switch (role)
		{
			case ROLE_SENDER:
				pubKey = pubKeySender;
				break;

			case ROLE_MIXER:
				pubKey = pubKeyMixer;
				break;

			case ROLE_GUARANTOR:
				pubKey = pubKeyGuarantor;
				break;
		}

		return pubKey;
	}

	CNode* GetNode(AnonymousTxRole role0) const
	{
		CNode* pN = NULL;
		switch (role0)
		{
			case ROLE_SENDER:
				pN = pSender;
				break;

			case ROLE_MIXER:
				pN = pMixer;
				break;

			case ROLE_GUARANTOR:
				pN = pGuarantor;
				break;
		}

		return pN;
	}

	std::vector<std::string> GetAllPubKeys() const
	{
		std::vector<std::string> vec;
		vec.push_back(pubKeySender);
		vec.push_back(pubKeyMixer);
		vec.push_back(pubKeyGuarantor);
		return vec;
	}

	void SetRole(AnonymousTxRole r)
	{
		role = r;
	}

	void SetNode(AnonymousTxRole role0, CNode* pN)
	{
		switch (role0)
		{
			case ROLE_SENDER:
				pSender = pN;
				break;

			case ROLE_MIXER:
				pMixer = pN;
				break;

			case ROLE_GUARANTOR:
				pGuarantor = pN;
				break;
		}
	}

	void SetAddressAndPubKey(AnonymousTxRole role0, std::string addr, std::string key)
	{
		switch (role0)
		{
			case ROLE_SENDER:
				addressSender = addr;
				pubKeySender = key;
				break;

			case ROLE_MIXER:
				addressMixer = addr;
				pubKeyMixer = key;
				break;

			case ROLE_GUARANTOR:
				addressGuarantor = addr;
				pubKeyGuarantor = key;
				break;
		}
	}

	bool IsPubKeyComplete() const
	{
		bool b = (pubKeySender != "") && (pubKeyMixer != "") && (pubKeyGuarantor != "");
		return b;
	}

	void clean()
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
};


class CAnonymousTxInfo
{
public:
	CAnonymousTxInfo()
	{
		status = ATX_STATUS_NONE;
		anonymousId = "";
		pParties = new AnonymousTxParties();
		lastActivityTime = GetTime();
		size = 0;
		pCoinControl = NULL;
		multiSigAddress = "";
		redeemScript = "";
		sendTx = "";
		committedMsTx = "";
		pMultiSigDistributionTx = new MultisigTxInfo();
	}

	virtual void clean(bool clearLog)
	{
		pParties->clean();
		size = 0;
		lastActivityTime = GetTime();
		status = ATX_STATUS_NONE;
		anonymousId = "";
		pCoinControl = NULL;
		multiSigAddress = "";
		redeemScript = "";
		sendTx = "";
		committedMsTx = "";

		pMultiSigDistributionTx->clean();

		if(clearLog)
			logs.clear();
	}

	bool IsNull() const
	{
		return (status == ATX_STATUS_NONE);
	}

	std::pair<std::string, int64_t> GetValue(int i)
	{
		return vecSendInfo.at(i);
	}

	int64_t GetLastActivityTime() const
	{
		return lastActivityTime;
	}

	const CCoinControl*	GetCoinControl() const
	{
		return pCoinControl;
	}

	AnonymousTxRole GetRole() const
	{
		return pParties->GetRole();
	}

	std::string GetSelfAddress() const
	{
		return pParties->GetSelfAddress();
	}

	int GetSize() const
	{
		return size;
	}

	std::string GetTx() const
	{
		return pMultiSigDistributionTx->GetTx();
	}

	std::vector< std::pair<std::string, int64_t> > GetSendInfo() const
	{
		return vecSendInfo;
	}

	std::string GetAddress(AnonymousTxRole role) const
	{
		return pParties->GetAddress(role);
	}

	std::string GetSelfPubKey() const
	{
		return pParties->GetSelfPubKey();
	}

	std::string GetAnonymousId() const
	{
		return anonymousId;
	}

	AnonymousTxStatus GetAtxStatus() const
	{
		return status;
	}

	CNode* GetNode(AnonymousTxRole role) const
	{
		return pParties->GetNode(role);
	}

	std::string GetNodeIpAddress(AnonymousTxRole role) const;

	std::vector<std::string> GetAllPubKeys() const
	{
		return pParties->GetAllPubKeys();
	}

	std::string GetMultiSigAddress() const
	{
		return multiSigAddress;
	}

	std::string GetRedeemScript() const
	{
		return redeemScript;
	}

	std::string GetTxid(AnonymousTxRole role) const
	{
		return pMultiSigDistributionTx->GetTxid(role);
	}

	int GetSignedCount() const
	{
		return pMultiSigDistributionTx->GetSignedCount();
	}

	void GetMultisigTxOutInfo(AnonymousTxRole role, std::string& txid, int& voutn, std::string& pubkey) const
	{
		pMultiSigDistributionTx->GetTxOutInfo(role, txid, voutn, pubkey);
	}

	std::string GetCommittedMsTx() const
	{
		return committedMsTx;
	}

	void SetLastActivityTime()
	{
		lastActivityTime = GetTime();
	}

	void SetAnonymousId(std::string aId)
	{
		lastActivityTime = GetTime();
		anonymousId = aId;

		if(status == ATX_STATUS_NONE)
			status = ATX_STATUS_RESERVE;
	}

	void SetSendTx(std::string tx)
	{
		sendTx = tx;
	}

	void SetNode(AnonymousTxRole role, CNode* pN)
	{
		pParties->SetNode(role, pN);
	}

	void SetCommittedMsTx(std::string tx)
	{
		lastActivityTime = GetTime();
		committedMsTx = tx;
		status = ATX_STATUS_COMPLETE;
	}

	void SetAddressAndPubKey(AnonymousTxRole role, std::string address, std::string key)
	{
		lastActivityTime = GetTime();
		pParties->SetAddressAndPubKey(role, address, key);

		if(pParties->IsPubKeyComplete())
			status = ATX_STATUS_PUBKEY;
	}

	void SetTxid(AnonymousTxRole role, std::string txid)
	{
		lastActivityTime = GetTime();
		pMultiSigDistributionTx->SetTxid(role, txid);

		if(pMultiSigDistributionTx->IsTxidComplete())
			status = ATX_STATUS_MSDEPO;
	}

	void SetVoutAndScriptPubKey(AnonymousTxRole role, int vout, std::string pubkey)
	{
		pMultiSigDistributionTx->SetVoutAndScriptPubKey(role, vout, pubkey);
	}

	void SetMultiSigAddress(std::string multiSigAddress0, std::string redeemScript0)
	{
		lastActivityTime = GetTime();
		m