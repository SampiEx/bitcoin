// Copyright (c) 2009-2011 Satoshi Nakamoto & Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file license.txt or http://www.opensource.org/licenses/mit-license.php.
#ifndef BITCOIN_KEYSTORE_H
#define BITCOIN_KEYSTORE_H

#include "crypter.h"

class CKeyStore
{
public:
    mutable CCriticalSection cs_KeyStore;

    virtual bool AddKey(const CKey& key) =0;
    virtual bool HaveKey(const CBitcoinAddress &address) const =0;
    virtual bool GetKey(const CBitcoinAddress &address, CKey& keyOut) const =0;
    virtual bool GetPubKey(const CBitcoinAddress &address, std::vector<unsigned char>& vchPubKeyOut) const;
    virtual std::vector<unsigned char> GenerateNewKey();
};

typedef std::map<CBitcoinAddress, CSecret> KeyMap;

class CBasicKeyStore : public CKeyStore
{
protected:
    KeyMap mapKeys;

public:
    bool AddKey(const CKey& key);
    bool HaveKey(const CBitcoinAddress &address) const
    {
        return (mapKeys.count(address) > 0);
    }
    bool GetKey(const CBitcoinAddress &address, CKey& keyOut) const
    {
        KeyMap::const_iterator mi = mapKeys.find(address);
        if (mi != mapKeys.end())
        {
            keyOut.SetSecret((*mi).second);
            return true;
        }
        return false;
    }
};

typedef std::map<CBitcoinAddress, std::pair<std::vector<unsigned char>, std::vector<unsigned char> > > CryptedKeyMap;

class CCryptoKeyStore : public CBasicKeyStore
{
private:
    CryptedKeyMap mapCryptedKeys;

    CKeyingMaterial vMasterKey;

    // if fUseCrypto is true, mapKeys must be empty
    // if fUseCrypto is false, vMasterKey must be empty
    bool fUseCrypto;

protected:
    bool SetCrypted()
    {
        if (fUseCrypto)
            return true;
        if (!mapKeys.empty())
            return false;
        fUseCrypto = true;
        return true;
    }

    // will encrypt previously unencrypted keys
    bool EncryptKeys(CKeyingMaterial& vMasterKeyIn);

    bool Unlock(const CKeyingMaterial& vMasterKeyIn);

public:
    mutable CCriticalSection cs_vMasterKey; //No guarantees master key wont get locked before you can use it, so lock this first

    CCryptoKeyStore() : fUseCrypto(false)
    {
    }

    bool IsCrypted() const
    {
        return fUseCrypto;
    }

    bool IsLocked() const
    {
        if (!IsCrypted())
            return false;
        return vMasterKey.empty();
    }

    bool Lock()
    {
        CRITICAL_BLOCK(cs_vMasterKey)
        {
            if (!SetCrypted())
                return false;

            vMasterKey.clear();
        }
        return true;
    }

    virtual bool AddCryptedKey(const std::vector<unsigned char> &vchPubKey, const std::vector<unsigned char> &vchCryptedSecret);
    std::vector<unsigned char> GenerateNewKey();
    bool AddKey(const CKey& key);
    bool HaveKey(const CBitcoinAddress &address) const
    {
        if (!IsCrypted())
            return CBasicKeyStore::HaveKey(address);
        return mapCryptedKeys.count(address) > 0;
    }
    bool GetKey(const CBitcoinAddress &address, CKey& keyOut) const;
    bool GetPubKey(const CBitcoinAddress &address, std::vector<unsigned char>& vchPubKeyOut) const;
};

#endif
