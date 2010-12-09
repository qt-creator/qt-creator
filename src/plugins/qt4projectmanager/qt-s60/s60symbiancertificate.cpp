/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "s60symbiancertificate.h"

#include <botan/x509_obj.h>
#include <botan/x509_ext.h>
#include <botan/der_enc.h>
#include <botan/ber_dec.h>
#include <botan/stl_util.h>
#include <botan/parsing.h>
#include <botan/bigint.h>
#include <botan/oids.h>
#include <botan/pem.h>
#include <algorithm>

using namespace Botan;

// ======== S60SymbianCertificatePrivate

class S60SymbianCertificatePrivate : private Botan::X509_Object
{
public:
    /**
   * Get the issuer certificate DN.
   * @return the issuer DN of this certificate
   */
    Botan::X509_DN issuerDn() const;

    /**
   * Get the subject certificate DN.
   * @return the subject DN of this certificate
   */
    Botan::X509_DN subjectDn() const;

    /**
   * Get a value for a specific subject_info parameter name.
   * @param name the name of the paramter to look up. Possible names are
   * "X509.Certificate.version", "X509.Certificate.serial",
   * "X509.Certificate.start", "X509.Certificate.end",
   * "X509.Certificate.v2.key_id", "X509.Certificate.public_key",
   * "X509v3.BasicConstraints.path_constraint",
   * "X509v3.BasicConstraints.is_ca", "X509v3.ExtendedKeyUsage",
   * "X509v3.CertificatePolicies", "X509v3.SubjectKeyIdentifier" or
   * "X509.Certificate.serial".
   * @return the value(s) of the specified parameter
   */
    std::vector<std::string> subjectInfo(const std::string& name) const;

    /**
   * Get a value for a specific subject_info parameter name.
   * @param name the name of the paramter to look up. Possible names are
   * "X509.Certificate.v2.key_id" or "X509v3.AuthorityKeyIdentifier".
   * @return the value(s) of the specified parameter
   */
    std::vector<std::string> issuerInfo(const std::string& name) const;

    /**
   * Get the notBefore of the certificate.
   * @return the notBefore of the certificate
   */
    std::string startTime() const;

    /**
   * Get the notAfter of the certificate.
   * @return the notAfter of the certificate
   */
    std::string endTime() const;

    /**
   * Get the X509 version of this certificate object.
   * @return the X509 version
   */
    Botan::u32bit x509Version() const;

    /**
   * Get the serial number of this certificate.
   * @return the certificates serial number
   */
    Botan::MemoryVector<Botan::byte> serialNumber() const;

    /**
   * Check whether this certificate is self signed.
   * @return true if this certificate is self signed
   */
    bool isSelfSigned() const { return m_selfSigned; }

    /**
   * Check whether this certificate is a CA certificate.
   * @return true if this certificate is a CA certificate
   */
    bool isCaCert() const;

    /**
   * Get the key constraints as defined in the KeyUsage extension of this
   * certificate.
   * @return the key constraints
   */
    Botan::Key_Constraints constraints() const;

    /**
   * Get the key constraints as defined in the ExtendedKeyUsage
   * extension of this
   * certificate.
   * @return the key constraints
   */
    std::vector<std::string> exConstraints() const;

    /**
   * Get the policies as defined in the CertificatePolicies extension
   * of this certificate.
   * @return the certificate policies
   */
    std::vector<std::string> policies() const;

    /**
   * Check to certificates for equality.
   * @return true both certificates are (binary) equal
   */
    bool operator==(const S60SymbianCertificatePrivate& other) const;

    /**
   * Create a certificate from a file containing the DER or PEM
   * encoded certificate.
   * @param filename the name of the certificate file
   */
    S60SymbianCertificatePrivate(const std::string& filename);

private:
    Botan::X509_DN createDn(const Botan::Data_Store& info) const;

protected:
    friend class X509_CA;
    S60SymbianCertificatePrivate() {}

    void force_decode();

protected:
    Botan::Data_Store m_subject;
    Botan::Data_Store m_issuer;
    bool m_selfSigned;
};

/*
* Lookup each OID in the vector
*/
std::vector<std::string> lookup_oids(const std::vector<std::string>& in)
{
    std::vector<std::string> out;

    std::vector<std::string>::const_iterator i = in.begin();
    while(i != in.end())
    {
        out.push_back(OIDS::lookup(OID(*i)));
        ++i;
    }
    return out;
}

/*
* S60SymbianCertificate Constructor
*/
S60SymbianCertificatePrivate::S60SymbianCertificatePrivate(const std::string& in) :
    X509_Object(in, "CERTIFICATE/X509 CERTIFICATE")
{
    m_selfSigned = false;

    do_decode();
}

/*
* Decode the TBSCertificate data
*/
void S60SymbianCertificatePrivate::force_decode()
{
    u32bit version;
    BigInt serial_bn;
    AlgorithmIdentifier sig_algo_inner;
    X509_DN dn_issuer, dn_subject;
    X509_Time start, end;

    BER_Decoder tbs_cert(tbs_bits);

    tbs_cert.decode_optional(version, ASN1_Tag(0),
                             ASN1_Tag(CONSTRUCTED | CONTEXT_SPECIFIC))
            .decode(serial_bn)
            .decode(sig_algo_inner)
            .decode(dn_issuer)
            .start_cons(SEQUENCE)
            .decode(start)
            .decode(end)
            .verify_end()
            .end_cons()
            .decode(dn_subject);

    if(version > 2)
        throw Decoding_Error("Unknown X.509 cert version " + to_string(version));
    if(sig_algo != sig_algo_inner)
        throw Decoding_Error("Algorithm identifier mismatch");

    m_selfSigned = (dn_subject == dn_issuer);

    m_subject.add(dn_subject.contents());
    m_issuer.add(dn_issuer.contents());

    BER_Object public_key = tbs_cert.get_next_object();
    if(public_key.type_tag != SEQUENCE || public_key.class_tag != CONSTRUCTED)
        throw Decoding_Error("Unexpected tag for public key");

    MemoryVector<byte> v2_issuer_key_id, v2_subject_key_id;

    tbs_cert.decode_optional_string(v2_issuer_key_id, BIT_STRING, 1);
    tbs_cert.decode_optional_string(v2_subject_key_id, BIT_STRING, 2);

    BER_Object v3_exts_data = tbs_cert.get_next_object();
    if(v3_exts_data.type_tag == 3 &&
            v3_exts_data.class_tag == ASN1_Tag(CONSTRUCTED | CONTEXT_SPECIFIC))
    {
        Extensions extensions(false);

        BER_Decoder(v3_exts_data.value).decode(extensions).verify_end();

        extensions.contents_to(m_subject, m_issuer);
    }
    else if(v3_exts_data.type_tag != NO_OBJECT)
        throw Decoding_Error("Unknown tag in X.509 cert");

    if(tbs_cert.more_items())
        throw Decoding_Error("TBSCertificate has more items that expected");

    m_subject.add("X509.Certificate.version", version);
    m_subject.add("X509.Certificate.serial", BigInt::encode(serial_bn));
    m_subject.add("X509.Certificate.start", start.readable_string());
    m_subject.add("X509.Certificate.end", end.readable_string());

    m_issuer.add("X509.Certificate.v2.key_id", v2_issuer_key_id);
    m_subject.add("X509.Certificate.v2.key_id", v2_subject_key_id);

    if(isCaCert() &&
            !m_subject.has_value("X509v3.BasicConstraints.path_constraint"))
    {
        u32bit limit = (x509Version() < 3) ? NO_CERT_PATH_LIMIT : 0;
        m_subject.add("X509v3.BasicConstraints.path_constraint", limit);
    }
}

/*
* Return the X.509 version in use
*/
u32bit S60SymbianCertificatePrivate::x509Version() const
{
    return (m_subject.get1_u32bit("X509.Certificate.version") + 1);
}

/*
* Return the time this cert becomes valid
*/
std::string S60SymbianCertificatePrivate::startTime() const
{
    return m_subject.get1("X509.Certificate.start");
}

/*
* Return the time this cert becomes invalid
*/
std::string S60SymbianCertificatePrivate::endTime() const
{
    return m_subject.get1("X509.Certificate.end");
}

/*
* Return information about the subject
*/
std::vector<std::string>
S60SymbianCertificatePrivate::subjectInfo(const std::string& what) const
{
    return m_subject.get(X509_DN::deref_info_field(what));
}

/*
* Return information about the issuer
*/
std::vector<std::string>
S60SymbianCertificatePrivate::issuerInfo(const std::string& what) const
{
    return m_issuer.get(X509_DN::deref_info_field(what));
}

/*
* Check if the certificate is for a CA
*/
bool S60SymbianCertificatePrivate::isCaCert() const
{
    if(!m_subject.get1_u32bit("X509v3.BasicConstraints.is_ca"))
        return false;
    if((constraints() & KEY_CERT_SIGN) || (constraints() == NO_CONSTRAINTS))
        return true;
    return false;
}

/*
* Return the key usage constraints
*/
Key_Constraints S60SymbianCertificatePrivate::constraints() const
{
    return Key_Constraints(m_subject.get1_u32bit("X509v3.KeyUsage",
                                                 NO_CONSTRAINTS));
}

/*
* Return the list of extended key usage OIDs
*/
std::vector<std::string> S60SymbianCertificatePrivate::exConstraints() const
{
    return lookup_oids(m_subject.get("X509v3.ExtendedKeyUsage"));
}

/*
* Return the list of certificate policies
*/
std::vector<std::string> S60SymbianCertificatePrivate::policies() const
{
    return lookup_oids(m_subject.get("X509v3.CertificatePolicies"));
}

/*
* Return the certificate serial number
*/
MemoryVector<byte> S60SymbianCertificatePrivate::serialNumber() const
{
    return m_subject.get1_memvec("X509.Certificate.serial");
}

/*
* Return the distinguished name of the issuer
*/
X509_DN S60SymbianCertificatePrivate::issuerDn() const
{
    return createDn(m_issuer);
}

/*
* Return the distinguished name of the subject
*/
X509_DN S60SymbianCertificatePrivate::subjectDn() const
{
    return createDn(m_subject);
}

/*
* Compare two certificates for equality
*/
bool S60SymbianCertificatePrivate::operator==(const S60SymbianCertificatePrivate& other) const
{
    return (sig == other.sig &&
            sig_algo == other.sig_algo &&
            m_selfSigned == other.m_selfSigned &&
            m_issuer == other.m_issuer &&
            m_subject == other.m_subject);
}

/*
* X.509 Certificate Comparison
*/
bool operator!=(const S60SymbianCertificatePrivate& cert1, const S60SymbianCertificatePrivate& cert2)
{
    return !(cert1 == cert2);
}

/*
* Create and populate a X509_DN
*/
X509_DN S60SymbianCertificatePrivate::createDn(const Data_Store& info) const
{
    class DN_Matcher : public Data_Store::Matcher
    {
    public:
        bool operator()(const std::string& key, const std::string&) const
        {
            if(key.find("X520.") != std::string::npos)
                return true;
            return false;
        }
    };

    std::multimap<std::string, std::string> names =
            info.search_with(DN_Matcher());

    X509_DN dn;

    std::multimap<std::string, std::string>::iterator j;
    for(j = names.begin(); j != names.end(); ++j)
        dn.add_attribute(j->first, j->second);

    return dn;
}

/*
* Create and populate an AlternativeName
*/
AlternativeName create_alt_name(const Data_Store& info)
{
    class AltName_Matcher : public Data_Store::Matcher
    {
    public:
        bool operator()(const std::string& key, const std::string&) const
        {
            for(u32bit j = 0; j != matches.size(); ++j)
                if(key.compare(matches[j]) == 0)
                    return true;
            return false;
        }

        AltName_Matcher(const std::string& match_any_of)
        {
            matches = split_on(match_any_of, '/');
        }
    private:
        std::vector<std::string> matches;
    };

    std::multimap<std::string, std::string> names =
            info.search_with(AltName_Matcher("RFC822/DNS/URI/IP"));

    AlternativeName alt_name;

    std::multimap<std::string, std::string>::iterator j;
    for(j = names.begin(); j != names.end(); ++j)
        alt_name.add_attribute(j->first, j->second);

    return alt_name;
}

// ======== S60SymbianCertificate
#include <QDebug>

S60SymbianCertificate::S60SymbianCertificate(const QString &filename) : m_d(0)
{
    S60SymbianCertificatePrivate *certificate = 0;
    try {
        certificate = new S60SymbianCertificatePrivate(filename.toStdString());
        m_d = certificate;
        certificate = 0;
    } catch (Botan::Exception &e) {
        m_errorString = QLatin1String(e.what());
    }
    delete certificate;
}

S60SymbianCertificate::~S60SymbianCertificate()
{
    delete m_d;
}

bool S60SymbianCertificate::isValid() const
{
    return m_d != 0;
}

QString S60SymbianCertificate::errorString() const
{
    return m_errorString;
}

QStringList S60SymbianCertificate::subjectInfo(const QString &name)
{
    Q_ASSERT(m_d);
    QStringList result;
    try {
        std::vector<std::string> subjectInfo(m_d->subjectInfo(name.toStdString()));
        std::vector<std::string>::const_iterator i;
        for(i = subjectInfo.begin(); i != subjectInfo.end(); ++i)
            result << QString::fromStdString(*i);
    } catch (Botan::Exception &e) {
            m_errorString = QString::fromLatin1(e.what());
    }
    return result;
}

QStringList S60SymbianCertificate::issuerInfo(const QString &name)
{
    Q_ASSERT(m_d);
    QStringList result;
    try {
        std::vector<std::string> issuerInfo(m_d->issuerInfo(name.toStdString()));

        std::vector<std::string>::const_iterator i;
        for(i = issuerInfo.begin(); i != issuerInfo.end(); ++i)
            result << QString::fromStdString(*i);
    } catch (Botan::Exception &e) {
            m_errorString = QString::fromLatin1(e.what());
    }
    return result;
}

QDateTime S60SymbianCertificate::parseTime(const std::string &time)
{
    QDateTime result;
    try {
        const char * const CERTIFICATE_DATE_FORMAT = "yyyy/M/d h:mm:ss UTC";
        QDateTime dateTime = QDateTime::fromString(QString::fromStdString(time),
                                                   QLatin1String(CERTIFICATE_DATE_FORMAT));
        result = QDateTime(dateTime.date(), dateTime.time(), Qt::UTC);
    } catch (Botan::Exception &e) {
        m_errorString = QString::fromLatin1(e.what());
    }
    return result;
}

QDateTime S60SymbianCertificate::startTime()
{
    Q_ASSERT(m_d);
    return parseTime(m_d->startTime());
}

QDateTime S60SymbianCertificate::endTime()
{
    Q_ASSERT(m_d);
    return parseTime(m_d->endTime());
}

quint32 S60SymbianCertificate::certificateVersion()
{
    Q_ASSERT(m_d);
    quint32 version = 0;
    try {
        version = static_cast<quint32>(m_d->x509Version());
    } catch (Botan::Exception &e) {
        m_errorString = QString::fromLatin1(e.what());
    }
    return version;
}

bool S60SymbianCertificate::isSelfSigned()
{
    Q_ASSERT(m_d);
    return m_d->isSelfSigned();
}

bool S60SymbianCertificate::isCaCert()
{
    Q_ASSERT(m_d);
    bool isCaCertificate = false;
    try {
        isCaCertificate = m_d->isCaCert();
    } catch (Botan::Exception &e) {
        m_errorString = QString::fromLatin1(e.what());
    }
    return isCaCertificate;
}
