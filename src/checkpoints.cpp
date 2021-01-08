// Copyright (c) 2009-2012 The Bitcoin developers
// Copyright (c) 2014 supercoindev
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <boost/assign/list_of.hpp> // for 'map_list_of()'
#include <boost/foreach.hpp>

#include "checkpoints.h"

#include "txdb.h"
#include "main.h"
#include "uint256.h"

namespace Checkpoints
{
    typedef std::map<int, uint256> MapCheckpoints;
	static int MAX_NO_SYNC_CHECKPOINT = 451000;

    //
    // What makes a good checkpoint block?
    // + Is surrounded by blocks with reasonable timestamps
    //   (no blocks before with a timestamp after, none after with
    //    timestamp before)
    // + Contains no strange transactions
    //
    static MapCheckpoints mapCheckpoints =
        boost::assign::map_list_of
        (     0, hashGenesisBlock )
        ( 10000, uint256("0xac79cf7b88660e130dfda4614502311eb4b3215c31a855a68932756fd09ade99"))
        ( 50000, uint256("0xfd8889dc7620ebf157d29bdd41bf4e2b0a1f689a452abc4603530efbc265f646"))
		(100000, uint256("0x00000000000d320bffd3fc5729e8ec0256961cec85f02d12dbf022528f9b2820"))
		(150000, uint256("0xea650f9ebcbb1a0769fa3c354cede69466afd491a1a106f76402713b4c93329e"))
	 	(200000, uint256("0x7a902d7a24da3769ea00910f79546b6d04833796604db30e79f06111b2e2e99a"))
		(250000, uint256("0x2e2df1883b133cdfee2563c291bd6a60646eddc02f7580a87439eca3b145ff3e"))
		(300000, uint256("0xc3f357f3f1474e43f927fe606d33938ab828666d9a2216bcc4f32ff58bf7173e")) 
		(350000, uint256("0xcfefea68aa35eae1240308bc7993beb390d8383ad9b0325e381ac3dd4a7fd237")) 
		(400000, uint256("0x56dde9a3bcd09341db6214a6397cae8e2b5b5d38cce758b8c8b88365c2d6198c")) 
		(413218, uint256("0x9959cc1968c4c0a6ff85beb6e9ab9b4ed0f658562f63ae7e0342a82112d20403")) 
		(420819, uint256("0x3371d3fcdd83da2edb89cfdef9b86453f7071e05ec0c1e9a1a1165f40c7d9228")) 
    ;

    // TestNet has no checkpoints
    static MapCheckpoints mapCheckpointsTestnet =
        boost::assign::map_list_of
        ( 0, hashGenesisBlockTestNet )
        ;

    bool CheckHardened(int nHeight, const uint256& hash)
    {
        MapCheckpoints& checkpoints = (fTestNet ? mapCheckpointsTestnet : mapCheckpoints);

        MapCheckpoints::const_iterator i = checkpoints.find(nHeight);
        if (i == checkpoints.end()) return true;
        return hash == i->second;
    }

    int GetTotalBlocksEstimate()
    {
        MapCheckpoints& checkpoints = (fTestNet ? mapCheckpointsTestnet : mapCheckpoints);

        return checkpoints.rbegin()->first;
    }

