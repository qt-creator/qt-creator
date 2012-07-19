/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

#include "s60symbiancertificate.h"

#include <botan/botan.h>

#include <algorithm>
#include <memory>
#include <string>

#include <QDateTime>

using namespace Botan;
using namespace Qt4ProjectManager;
using namespace Qt4ProjectManager::Internal;

namespace {
    const char CERT_IMEI_FIELD_NAME[] = "1.2.826.0.1.1796587.1.1.1.1";
    const char CERT_CAPABILITY_FIELD_NAME[] = "1.2.826.0.1.1796587.1.1.1.6";
}

// ======== S60CertificateExtension

/*
* X.509 S60 Certificate Extension
*/
class S60CertificateExtension : public Certificate_Extension
{
public:
    OID oid_of() const;

    virtual S60CertificateExtension* copy() const = 0;
    virtual ~S60CertificateExtension() {}
protected:
    friend class S60Extensions;
    virtual bool should_encode() const { return false; }
    virtual MemoryVector<byte> encode_inner() const = 0;
    virtual void decode_inner(const MemoryRegion<byte>&) = 0;
};

// ======== S60DeviceIdListConstraint

class S60DeviceIdListConstraint : public S60CertificateExtension
{
public:
    S60CertificateExtension* copy() const { return new S60DeviceIdListConstraint(imei_list); }

    S60DeviceIdListConstraint() {}
    S60DeviceIdListConstraint(const std::vector<ASN1_String>& o) : imei_list(o) {}

    std::vector<ASN1_String> get_imei_list() const { return imei_list; }
private:
    std::string config_id() const { return "deviceid_list_constraint"; }
    std::string oid_name() const { return CERT_IMEI_FIELD_NAME; }

    MemoryVector<byte> encode_inner() const;
    void decode_inner(const MemoryRegion<byte>&);
    void contents_to(Data_Store&, Data_Store&) const;

    std::vector<ASN1_String> imei_list;
};

/*
* Encode the extension
*/
MemoryVector<byte> S60DeviceIdListConstraint::encode_inner() const
{
    qFatal("Encoding S60 extensions is not supported.");
    return MemoryVector<byte>();
}

/*
* Decode the extension
*/
void S60DeviceIdListConstraint::decode_inner(const MemoryRegion<byte>& in)
{
    BER_Decoder(in)
            .start_cons(SEQUENCE)
            .decode_list(imei_list)
            .end_cons();
}

/*
* Return a textual representation
*/
void S60DeviceIdListConstraint::contents_to(Data_Store& subject, Data_Store&) const
{
    for(u32bit j = 0; j != imei_list.size(); ++j)
        subject.add(CERT_IMEI_FIELD_NAME, imei_list[j].value());
}

// ======== S60CapabilityConstraint

class S60CapabilityConstraint : public S60CertificateExtension
{
public:
    S60CertificateExtension* copy() const { return new S60CapabilityConstraint(capabilities); }

    S60CapabilityConstraint() {}
    S60CapabilityConstraint(const SecureVector<byte>& o) : capabilities(o) {}

    SecureVector<byte> get_capability_list() const { return capabilities; }
private:
    std::string config_id() const { return "capability_constraint"; }
    std::string oid_name() const { return "CERT_CAPABILITY_FIELD_NAME"; }

    MemoryVector<byte> encode_inner() const;
    void decode_inner(const MemoryRegion<byte>&);
    void contents_to(Data_Store&, Data_Store&) const;

    SecureVector<byte> capabilities;
};

/*
* Encode the extension
*/
MemoryVector<byte> S60CapabilityConstraint::encode_inner() const
{
    qFatal("Encoding S60 extensions is not supported.");
    return MemoryVector<byte>();
}

/*
* Decode the extension
*/
void S60CapabilityConstraint::decode_inner(const MemoryRegion<byte>& in)
{
    BER_Decoder(in)
            .decode(capabilities, BIT_STRING)
            .verify_end();
}

/*
* Return a textual representation
*/
void S60CapabilityConstraint::contents_to(Data_Store& subject, Data_Store&) const
{
    quint32 capabilitiesValue = 0;
    for(u32bit j = 0; j != sizeof(quint32); ++j) {
        quint32 capabilitie(capabilities[sizeof(quint32)-1-j]);
        capabilitiesValue |= capabilitie << 8*j;
    }
    subject.add(CERT_CAPABILITY_FIELD_NAME, capabilitiesValue);
}

// ======== S60Extensions

class S60Extensions : public ASN1_Object
{
public:
    void encode_into(class DER_Encoder&) const;
    void decode_from(class BER_Decoder&);

    void contents_to(Data_Store&, Data_Store&) const;

    void add(Certificate_Extension* extn)
    { extensions.push_back(extn); }

    S60Extensions& operator=(const S60Extensions&);

    S60Extensions(const S60Extensions&);
    S60Extensions(bool st = true) : should_throw(st) {}
    ~S60Extensions();
private:
    static Certificate_Extension* get_extension(const OID&);

    std::vector<Certificate_Extension*> extensions;
    bool should_throw;
};

/*
* S60Extensions Copy Constructor
*/
S60Extensions::S60Extensions(const S60Extensions& extensions) : ASN1_Object()
{
    *this = extensions;
}

/*
* Extensions Assignment Operator
*/
S60Extensions& S60Extensions::operator=(const S60Extensions& other)
{
    for(u32bit j = 0; j != extensions.size(); ++j)
        delete extensions[j];
    extensions.clear();

    for(u32bit j = 0; j != other.extensions.size(); ++j)
        extensions.push_back(other.extensions[j]->copy());

    return (*this);
}

/*
* Return the OID of this extension
*/
OID S60CertificateExtension::oid_of() const
{
    return OIDS::lookup(oid_name());
}

/*
* Encode an Extensions list
*/
void S60Extensions::encode_into(DER_Encoder& to_object) const
{
    Q_UNUSED(to_object);
    qFatal("Encoding S60 extensions is not supported.");
}

/*
* Decode a list of Extensions
*/
void S60Extensions::decode_from(BER_Decoder& from_source)
{
    for(u32bit j = 0; j != extensions.size(); ++j)
        delete extensions[j];
    extensions.clear();

    BER_Decoder sequence = from_source.start_cons(SEQUENCE);
    while(sequence.more_items())
    {
        OID oid;
        MemoryVector<byte> value;
        bool critical;

        sequence.start_cons(SEQUENCE)
                .decode(oid)
                .decode_optional(critical, BOOLEAN, UNIVERSAL, false)
                .decode(value, OCTET_STRING)
                .verify_end()
                .end_cons();

        S60CertificateExtension* ext = 0;
        if (OIDS::name_of(oid, CERT_IMEI_FIELD_NAME))
            ext = new S60DeviceIdListConstraint();

        if (OIDS::name_of(oid, CERT_CAPABILITY_FIELD_NAME))
            ext = new S60CapabilityConstraint();

        if (!ext) {
            if (!critical || !should_throw)
                continue;

            throw Decoding_Error("Encountered unknown X.509 extension marked "
                                 "as critical; OID = " + oid.as_string());
        }
        ext->decode_inner(value);
        extensions.push_back(ext);
    }
    sequence.verify_end();
}

/*
* Write the extensions to an info store
*/
void S60Extensions::contents_to(Data_Store& subject_info,
                                Data_Store& issuer_info) const
{
    for(u32bit j = 0; j != extensions.size(); ++j)
        extensions[j]->contents_to(subject_info, issuer_info);
}

/*
* Delete an Extensions list
*/
S60Extensions::~S60Extensions()
{
    for(u32bit j = 0; j != extensions.size(); ++j)
        delete extensions[j];
}



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
   * @param name the name of the parameter to look up. Possible names are
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
   * @param name the name of the parameter to look up. Possible names are
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
    S60SymbianCertificatePrivate(const QByteArray &filename);

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
S60SymbianCertificatePrivate::S60SymbianCertificatePrivate(const QByteArray &in) :
    X509_Object(in.constData(), "CERTIFICATE/X509 CERTIFICATE")
{
    m_selfSigned = false;

    do_decode();
}

/*
* Decode the TBSCertificate data
*/
void S60SymbianCertificatePrivate::force_decode()
{
    size_t version;
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
        S60Extensions s60extensions(false);

        BER_Decoder(v3_exts_data.value).decode(s60extensions).verify_end();

        s60extensions.contents_to(m_subject, m_issuer);

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
        u32bit limit = (x509Version() < 3) ? Cert_Extension::NO_CERT_PATH_LIMIT : 0;
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

S60SymbianCertificate::S60SymbianCertificate(const QString &filename) : d(0)
{
    S60SymbianCertificatePrivate *certificate = 0;
    try {
        certificate = new S60SymbianCertificatePrivate(filename.toLatin1());
        d = certificate;
        certificate = 0;
    } catch (Botan::Exception &e) {
        m_errorString = QLatin1String(e.what());
    }
    delete certificate;
}

S60SymbianCertificate::~S60SymbianCertificate()
{
    delete d;
}

bool S60SymbianCertificate::isValid() const
{
    return d != 0;
}

QString S60SymbianCertificate::errorString() const
{
    return m_errorString;
}

QStringList S60SymbianCertificate::subjectInfo(const QString &name)
{
    Q_ASSERT(d);
    QStringList result;
    try {
        std::vector<std::string> subjectInfo =
            d->subjectInfo(name.toLatin1().constData());
        std::vector<std::string>::const_iterator i;
        for (i = subjectInfo.begin(); i != subjectInfo.end(); ++i)
            result << QString::fromLatin1(i->c_str());
    } catch (Botan::Exception &e) {
        m_errorString = QString::fromLatin1(e.what());
    }
    return result;
}

QStringList S60SymbianCertificate::issuerInfo(const QString &name)
{
    Q_ASSERT(d);
    QStringList result;
    try {
        std::vector<std::string> issuerInfo =
            d->issuerInfo(name.toLatin1().constData());

        std::vector<std::string>::const_iterator i;
        for (i = issuerInfo.begin(); i != issuerInfo.end(); ++i)
            result << QString::fromLatin1(i->c_str());
    } catch (Botan::Exception &e) {
        m_errorString = QString::fromLatin1(e.what());
    }
    return result;
}

QDateTime S60SymbianCertificate::parseTime(const QByteArray &time)
{
    QDateTime result;
    try {
        const char * const CERTIFICATE_DATE_FORMAT = "yyyy/M/d h:mm:ss UTC";
        QDateTime dateTime = QDateTime::fromString(QString::fromLatin1(time),
                                                   QLatin1String(CERTIFICATE_DATE_FORMAT));
        result = QDateTime(dateTime.date(), dateTime.time(), Qt::UTC);
    } catch (Botan::Exception &e) {
        m_errorString = QString::fromLatin1(e.what());
    }
    return result;
}

QDateTime S60SymbianCertificate::startTime()
{
    Q_ASSERT(d);
    return parseTime(d->startTime().c_str());
}

QDateTime S60SymbianCertificate::endTime()
{
    Q_ASSERT(d);
    return parseTime(d->endTime().c_str());
}

quint32 S60SymbianCertificate::certificateVersion()
{
    Q_ASSERT(d);
    quint32 version = 0;
    try {
        version = static_cast<quint32>(d->x509Version());
    } catch (Botan::Exception &e) {
        m_errorString = QString::fromLatin1(e.what());
    }
    return version;
}

bool S60SymbianCertificate::isSelfSigned()
{
    Q_ASSERT(d);
    return d->isSelfSigned();
}

bool S60SymbianCertificate::isCaCert()
{
    Q_ASSERT(d);
    bool isCaCertificate = false;
    try {
        isCaCertificate = d->isCaCert();
    } catch (Botan::Exception &e) {
        m_errorString = QString::fromLatin1(e.what());
    }
    return isCaCertificate;
}
