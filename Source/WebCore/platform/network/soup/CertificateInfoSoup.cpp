/*
 * Copyright (C) 2012 Igalia S.L.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#if USE(SOUP)

#include "CertificateInfo.h"

#include <ResourceError.h>
#include <ResourceResponse.h>
#include <libsoup/soup.h>
#include <wtf/glib/GSpanExtras.h>

namespace WebCore {

CertificateInfo::CertificateInfo()
    : m_tlsErrors(static_cast<GTlsCertificateFlags>(0))
{
}

CertificateInfo::CertificateInfo(const ResourceResponse& response)
    : m_certificate(response.soupMessageCertificate())
    , m_tlsErrors(response.soupMessageTLSErrors())
{
}

CertificateInfo::CertificateInfo(const ResourceError& resourceError)
    : m_certificate(resourceError.certificate())
    , m_tlsErrors(static_cast<GTlsCertificateFlags>(resourceError.tlsErrors()))
{
}

CertificateInfo::CertificateInfo(GRefPtr<GTlsCertificate>&& certificate, GTlsCertificateFlags tlsErrors)
    : m_certificate(WTFMove(certificate))
    , m_tlsErrors(tlsErrors)
{
}

CertificateInfo::~CertificateInfo() = default;

CertificateInfo CertificateInfo::isolatedCopy() const
{
    if (!m_certificate)
        return { };

    Vector<GUniquePtr<char>> certificatesDataList;
    for (auto* nextCertificate = m_certificate.get(); nextCertificate; nextCertificate = g_tls_certificate_get_issuer(nextCertificate)) {
        GUniqueOutPtr<char> certificateData;
        g_object_get(nextCertificate, "certificate-pem", &certificateData.outPtr(), nullptr);
        certificatesDataList.append(certificateData.release());
    }

    GUniqueOutPtr<char> privateKey;
    GUniqueOutPtr<char> privateKeyPKCS11Uri;
    g_object_get(m_certificate.get(), "private-key-pem", &privateKey.outPtr(), "private-key-pkcs11-uri", &privateKeyPKCS11Uri.outPtr(), nullptr);

    GType certificateType = g_tls_backend_get_certificate_type(g_tls_backend_get_default());
    GRefPtr<GTlsCertificate> certificate;
    GTlsCertificate* issuer = nullptr;
    while (!certificatesDataList.isEmpty()) {
        auto certificateData = certificatesDataList.takeLast();
        certificate = adoptGRef(G_TLS_CERTIFICATE(g_initable_new(
            certificateType, nullptr, nullptr,
            "certificate-pem", certificateData.get(),
            "issuer", issuer,
            "private-key-pem", certificatesDataList.isEmpty() ? privateKey.get() : nullptr,
            "private-key-pkcs11-uri", certificatesDataList.isEmpty() ? privateKeyPKCS11Uri.get() : nullptr,
            nullptr)));
        RELEASE_ASSERT(certificate);
        issuer = certificate.get();
    }

    return CertificateInfo(certificate.get(), m_tlsErrors);
}

std::optional<CertificateSummary> CertificateInfo::summary() const
{
    if (!m_certificate)
        return std::nullopt;

    CertificateSummary summaryInfo;

    GRefPtr<GDateTime> validNotBefore;
    GRefPtr<GDateTime> validNotAfter;
    GUniqueOutPtr<char> subjectName;
    GRefPtr<GPtrArray> dnsNames;
    GRefPtr<GPtrArray> ipAddresses;
    g_object_get(m_certificate.get(), "not-valid-before", &validNotBefore.outPtr(), "not-valid-after", &validNotAfter.outPtr(),
        "subject-name", &subjectName.outPtr(), "dns-names", &dnsNames.outPtr(), "ip-addresses", &ipAddresses.outPtr(), nullptr);

    if (validNotBefore)
        summaryInfo.validFrom = Seconds(static_cast<double>(g_date_time_to_unix(validNotBefore.get())));
    if (validNotAfter)
        summaryInfo.validUntil = Seconds(static_cast<double>(g_date_time_to_unix(validNotAfter.get())));
    if (subjectName)
        summaryInfo.subject = String::fromUTF8(subjectName.get());
    for (auto dnsName : span<GBytes*>(dnsNames))
        summaryInfo.dnsNames.append(String(span(dnsName)));
    for (auto address : span<GInetAddress*>(ipAddresses)) {
        GUniquePtr<char> ipAddress(g_inet_address_to_string(address));
        summaryInfo.ipAddresses.append(String::fromUTF8(ipAddress.get()));
    }

    return summaryInfo;
}

} // namespace WebCore

#endif
