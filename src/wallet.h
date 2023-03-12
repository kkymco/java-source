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
		multiSigAddress = multiSigAddress0;
		redeemScript = redeemScript0;
		status = ATX_STATUS_MSADDR;
	}

	void SetTx(std::string tx, int sc)
	{
		lastActivityTime = GetTime();

		if(sc == 0 && status < ATX_STATUS_MSTXR0)
		{
			status = ATX_STATUS_MSTXR0;
			pMultiSigDistributionTx->SetTx(tx, sc);
		}
		else if(sc == 1 && status < ATX_STATUS_MSTXR1)
		{
			status = ATX_STATUS_MSTXR1;
			pMultiSigDistributionTx->SetTx(tx, sc);
		}
		else if(sc == 2 && status < ATX_STATUS_MSTXRC)
		{
			status = ATX_STATUS_MSTXRC;
			pMultiSigDistributionTx->SetTx(tx, sc);
		}
	}

	void SetNewData(std::string anonymousId0, CNode* pMixerNode, CNode* pGuarantorNode)
	{
		lastActivityTime = GetTime();
		anonymousId = anonymousId0;
		pParties->SetNode(ROLE_MIXER, pMixerNode);
		pParties->SetNode(ROLE_GUARANTOR, pGuarantorNode);
	}

	bool SetInitialData(AnonymousTxRole role, std::vector< std::pair<std::string, int64_t> > vecSendInfo, const CCoinControl* pCoinControl,
		CNode* pSendNode, CNode* pMixerNode, CNode* pGuarantorNode, CWallet* pWallet);

	bool CanReset() const;
	int64_t GetTotalRequiredCoinsToSend(AnonymousTxRole role = ROLE_UNKNOWN);

	bool CheckDepositTxes(CWallet* pWallet);
	bool CheckSendTx(CWallet* pWallet);
	bool IsCurrentTxInProcess() const;
	void AddToLog(std::string text);
	std::string GetLastAnonymousTxLog();

private:
	bool CheckDeposit(AnonymousTxRole role, CWallet* pWallet);
	int64_t GetDepositedAmount(CTransaction tx);

	AnonymousTxStatus			status;		
	std::string					anonymousId;
	AnonymousTxParties*			pParties;
	int64_t						lastActivityTime;
	int							size;
	const CCoinControl*			pCoinControl;
	std::string					multiSigAddress;
	std::string					redeemScript;
	std::string					sendTx;
	MultisigTxInfo*				pMultiSigDistributionTx;
	std::string					committedMsTx;

	std::vector< std::pair<std::string, int64_t> > vecSendInfo;
	std::vector<std::string>	logs;
};


/** A CWallet is an extension of a keystore, which also maintains a set of transactions and balances,
 * and provides the ability to create new transactions.
 */
class CWallet : public CCryptoKeyStore
{
private:
    bool SelectCoinsSimple(int64_t nTargetValue, unsigned int nSpendTime, int nMinConf, std::set<std::pair<const CWalletTx*,unsigned int> >& setCoinsRet, int64_t& nValueRet) const;
    bool SelectCoins(int64_t nTargetValue, unsigned int nSpendTime, std::set<std::pair<const CWalletTx*,unsigned int> >& setCoinsRet, int64_t& nValueRet, const CCoinControl *coinControl=NULL) const;

    CWalletDB *pwalletdbEncryption;

    // the current wallet version: clients below this version are not able to load the wallet
    int nWalletVersion;

    // the maximum wallet format version: memory-only variable that specifies to what version this wallet may be upgraded
    int nWalletMaxVersion;

	// current anonymous send info (only allow one for now, for one sender)
	std::string selfAddress;
	CAnonymousTxInfo* pCurrentAnonymousTxInfo;


public:
    mutable CCriticalSection cs_wallet;
    mutable CCriticalSection cs_servicelist;

    bool fFileBacked;
    std::string strWalletFile, strDonationsFile;

    std::set<int64_t> setKeyPool;

    std::map<CKeyID, CKeyMetadata> mapKeyMetadata;


    typedef std::map<unsigned int, CMasterKey> MasterKeyMap;
    MasterKeyMap mapMasterKeys;
    unsigned int nMasterKeyMaxID;

    CWallet()
    {
        nWalletVersion = FEATURE_BASE;
        nWalletMaxVersion = FEATURE_BASE;
        fFileBacked = false;
        nMasterKeyMaxID = 0;
        pwalletdbEncryption = NULL;
        nOrderPosNext = 0;
		pCurrentAnonymousTxInfo = new CAnonymousTxInfo();
		selfAddress = "";
		mapAnonymousServices.clear();
    }
    CWallet(std::string strWalletFileIn)
    {
        nWalletVersion = FEATURE_BASE;
        nWalletMaxVersion = FEATURE_BASE;
        strWalletFile = strWalletFileIn;
        fFileBacked = true;
        nMasterKeyMaxID = 0;
        pwalletdbEncryption = NULL;
        nOrderPosNext = 0;
		pCurrentAnonymousTxInfo = new CAnonymousTxInfo();
		selfAddress = "";
		mapAnonymousServices.clear();
    }

	~CWallet()
	{
		delete pCurrentAnonymousTxInfo;
    }

    std::map<uint256, CWalletTx> mapWallet;
    int64_t nOrderPosNext;
    std::map<uint256, int> mapRequestCount;

    std::map<CTxDestination, std::string> mapAddressBook;
	std::map<std::string, std::string> mapAnonymousServices;

    CPubKey vchDefaultKey;
    int64_t nTimeFirstKey;

    // check whether we are allowed to upgrade (or already support) to the named feature
    bool CanSupportFeature(enum WalletFeature wf) { return nWalletMaxVersion >= wf; }

    void AvailableCoinsMinConf(std::vector<COutput>& vCoins, int nConf) const;
    void AvailableCoins(std::vector<COutput>& vCoins, bool fOnlyConfirmed=true, const CCoinControl *coinControl=NULL) const;
    bool SelectCoinsMinConf(int64_t nTargetValue, unsigned int nSpendTime, int nConfMine, int nConfTheirs, std::vector<COutput> vCoins, std::set<std::pair<const CWalletTx*,unsigned int> >& setCoinsRet, int64_t& nValueRet) const;
    // keystore implementation
    // Generate a new key
    CPubKey GenerateNewKey();
    // Adds a key to the store, and saves it to disk.
    bool AddKey(const CKey& key);
    // Adds a key to the store, without saving it to disk (used by LoadWallet)
    bool LoadKey(const CKey& key) { return CCryptoKeyStore::AddKey(key); }
    // Load metadata (used by LoadWallet)
    bool LoadKeyMetadata(const CPubKey &pubkey, const CKeyMetadata &metadata);

    bool LoadMinVersion(int nVersion) { nWalletVersion = nVersion; nWalletMaxVersion = std::max(nWalletMaxVersion, nVersion); return true; }

    // Adds an encrypted key to the store, and saves it to disk.
    bool AddCryptedKey(const CPubKey &vchPubKey, const std::vector<unsigned char> &vchCryptedSecret);
    // Adds an encrypted key to the store, without saving it to disk (used by LoadWallet)
    bool LoadCryptedKey(const CPubKey &vchPubKey, const std::vector<unsigned char> &vchCryptedSecret);
    bool AddCScript(const CScript& redeemScript);
    bool LoadCScript(const CScript& redeemScript) { return CCryptoKeyStore::AddCScript(redeemScript); }

    bool Unlock(const SecureString& strWalletPassphrase);
    bool ChangeWalletPassphrase(const SecureString& strOldWalletPassphrase, const SecureString& strNewWalletPassphrase);
    bool EncryptWallet(const SecureString& strWalletPassphrase);

    void GetKeyBirthTimes(std::map<CKeyID, int64_t> &mapKeyBirth) const;


    /** Increment the next transaction order id
        @return next transaction order id
     */
    int64_t IncOrderPosNext(CWalletDB *pwalletdb = NULL);

    typedef std::pair<CWalletTx*, CAccountingEntry*> TxPair;
    typedef std::multimap<int64_t, TxPair > TxItems;

    /** Get the wallet's activity log
        @return multimap of ordered transactions and accounting entries
        @warning Returned pointers are *only* valid within the scope of passed acentries
     */
    TxItems OrderedTxItems(std::list<CAccountingEntry>& acentries, std::string strAccount = "");

    void MarkDirty();
    bool AddToWallet(const CWalletTx& wtxIn);
    bool AddToWalletIfInvolvingMe(const CTransaction& tx, const CBlock* pblock, bool fUpdate = false, bool fFindBlock = false);
    bool EraseFromWallet(uint256 hash);
    void WalletUpdateSpent(const CTransaction& prevout, bool fBlock = false);
    int ScanForWalletTransactions(CBlockIndex* pindexStart, bool fUpdate = false);
    int ScanForWalletTransaction(const uint256& hashTx);
    void ReacceptWalletTransactions();
    void ResendWalletTransactions(bool fForce = false);
    int64_t GetBalance() const;
    int64_t GetUnconfirmedBalance() const;
    int64_t GetImmatureBalance() const;
    int64_t GetStake() const;
    int64_t GetNewMint() const;
    bool CreateTransaction(const std::vector<std::pair<CScript, int64_t> >& vecSend, CWalletTx& wtxNew, CReserveKey& reservekey, int64_t& nFeeRet, const CCoinControl *coinControl=NULL);
    bool CreateTransaction(CScript scriptPubKey, int64_t nValue, CWalletTx& wtxNew, CReserveKey& reservekey, int64_t& nFeeRet, const CCoinControl *coinControl=NULL);
    bool CommitTransaction(CWalletTx& wtxNew, CReserveKey& reservekey);

    bool GetStakeWeight(const CKeyStore& keystore, uint64_t& nMinWeight, uint64_t& nMaxWeight, uint64_t& nWeight);
    bool CreateCoinStake(const CKeyStore& keystore, unsigned int nBits, int64_t nSearchInterval, int64_t nFees, CTransaction& txNew, CKey& key);

    std::string SendMoney(CScript scriptPubKey, int64_t nValue, CWalletTx& wtxNew, bool fAskFee=false);
    std::string SendMoneyToDestination(const CTxDestination &address, int64_t nValue, CWalletTx& wtxNew, bool fAskFee=false);

    bool NewKeyPool();
    bool TopUpKeyPool(unsigned int nSize = 0);
    int64_t AddReserveKey(const CKeyPool& keypool);
    void ReserveKeyFromKeyPool(int64_t& nIndex, CKeyPool& keypool);
    void KeepKey(int64_t nIndex);
    void ReturnKey(int64_t nIndex);
    bool GetKeyFromPool(CPubKey &key, bool fAllowReuse=true);
    int64_t GetOldestKeyPoolTime();
    void GetAllReserveKeys(std::set<CKeyID>& setA