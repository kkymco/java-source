#include "donation.h"
#include "util.h"
#include "main.h"
#include <math.h>
#include <boost/cstdint.hpp>
#include <boost/lexical_cast.hpp>
#include <vector>
#include "wallet.h"
#include "walletdb.h"
#include "bitcoinrpc.h"
#include "init.h"
#include "base58.h"

using namespace json_spirit;
using namespace std;

std::map<uint256,double> ConfirmedBlocksWaitingOnDonate;

double CalcDonationAmount()
{
    double nDonAmnt = 0;

    double nPercent = nDonatePercent;
    if (nPercent < 0.0)
    {
        nPercent = 0.0;
    }
    else if (nPercent > 100.0)
    {
        nPercent = 100.0;
    }
    double dbPDV = (double)PDV;

    nDonAmnt =  ((dbPDV - MIN_TX_FEE )* (nPercent * 0.01));  // takes the amount that was earned and removes tx fee before calcing percent
    // this ensures that at even 100% donation, the user isnt slowly losing coins.

    return nDonAmnt;
}

std::string getUsableAddress(double amountRequired)
{
    int nMinDepth = 1000;
    std::string sAccount;
    map<string, int64_t> mapAccountBalances;
    BOOST_FOREACH(const PAIRTYPE(CTxDestination, string)& entry, pwalletMain->mapAddressBook)
    {
        if (IsMine(*pwalletMain, entry.first)) // This address belongs to me
        {
            mapAccountBalances[entry.second] = 0;
        }
    }
    for (map<uint256, CWalletTx>::iterator it = pwalletMain->mapWallet.begin(); it != pwalletMain->mapWallet.end(); ++it)
    {
        const CWalletTx& wtx = (*it).second;
        int64_t nFee;
        string strSentAccount;
        list<pair<CTxDestination, int64_t> > listReceived;
        list<pair<CTxDestination, int64_t> > listSent;
        int nDepth = wtx.GetDepthInMainChain();
        if (nDepth < 0)
        {
            continue;
        }
        wtx.GetAmounts(listReceived, listSent, nFee, strSentAccount);
        mapAccountBalances[strSentAccount] -= nFee;
        BOOST_FOREACH(const PAIRTYPE(CTxDestination, int64_t)& s, listSent)
            mapAccountBalances[strSentAccount] -= s.second;
        if (nDepth >= nMinDepth && wtx.GetBlocksToM