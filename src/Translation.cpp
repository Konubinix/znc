/*
 * Copyright (C) 2004-2016 ZNC, see the NOTICE file for details.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <znc/Translation.h>

#ifdef HAVE_I18N
#include <boost/locale.hpp>
#endif

CTranslation& CTranslation::Get() {
    static CTranslation translation;
    return translation;
}

CString CTranslation::Singular(const CString& sDomain, const CString& sContext,
                               const CString& sEnglish) {
#ifdef HAVE_I18N
    const std::locale& loc = LoadTranslation(sDomain);
    return boost::locale::translate(sContext, sEnglish).str(loc);
#else
    return sEnglish;
#endif
}
CString CTranslation::Plural(const CString& sDomain, const CString& sContext,
                             const CString& sEnglish, const CString& sEnglishes,
                             int iNum) {
#ifdef HAVE_I18N
    const std::locale& loc = LoadTranslation(sDomain);
    return boost::locale::translate(sContext, sEnglish, sEnglishes, iNum)
        .str(loc);
#else
    if (iNum == 1) {
        return sEnglish;
    } else {
        return sEnglishes;
    }
#endif
}

const std::locale& CTranslation::LoadTranslation(const CString& sDomain) {
    CString sLanguage = m_sLanguageStack.empty() ? "" : m_sLanguageStack.back();
#ifdef HAVE_I18N
    // Not using built-in support for multiple domains in single std::locale
    // via overloaded call to .str() because we need to be able to reload
    // translations from disk independently when a module gets updated
    auto& domain = m_Translations[sDomain];
    auto lang_it = domain.find(sLanguage);
    if (lang_it == domain.end()) {
        boost::locale::generator gen;
        gen.add_messages_path(LOCALE_DIR);
        gen.add_messages_domain(sDomain);
        std::tie(lang_it, std::ignore) =
            domain.emplace(sLanguage, gen(sLanguage + ".UTF-8"));
    }
    return lang_it->second;
#else
    // dummy, it's not used anyway
    return std::locale::classic();
#endif
}

void CTranslation::PushLanguage(const CString& sLanguage) {
    m_sLanguageStack.push_back(sLanguage);
}
void CTranslation::PopLanguage() { m_sLanguageStack.pop_back(); }

void CTranslation::NewReference(const CString& sDomain) {
    m_miReferences[sDomain]++;
}
void CTranslation::DelReference(const CString& sDomain) {
    if (!--m_miReferences[sDomain]) {
        m_Translations.erase(sDomain);
    }
}

CString CCoreTranslationMixin::t(const CString& sEnglish,
                                 const CString& sContext) {
    return CTranslation::Get().Singular("znc", sContext, sEnglish);
}

CInlineFormatMessage CCoreTranslationMixin::f(const CString& sEnglish,
                                              const CString& sContext) {
    return CInlineFormatMessage(t(sEnglish, sContext));
}

CInlineFormatMessage CCoreTranslationMixin::p(const CString& sEnglish,
                                              const CString& sEnglishes,
                                              int iNum,
                                              const CString& sContext) {
    return CInlineFormatMessage(CTranslation::Get().Plural(
        "znc", sContext, sEnglish, sEnglishes, iNum));
}

CDelayedTranslation CCoreTranslationMixin::d(const CString& sEnglish,
                                             const CString& sContext) {
    return CDelayedTranslation("znc", sContext, sEnglish);
}


CLanguageScope::CLanguageScope(const CString& sLanguage) {
    CTranslation::Get().PushLanguage(sLanguage);
}
CLanguageScope::~CLanguageScope() { CTranslation::Get().PopLanguage(); }

CString CDelayedTranslation::Resolve() const {
    return CTranslation::Get().Singular(m_sDomain, m_sContext, m_sEnglish);
}

CTranslationDomainRefHolder::CTranslationDomainRefHolder(const CString& sDomain)
    : m_sDomain(sDomain) {
    CTranslation::Get().NewReference(sDomain);
}
CTranslationDomainRefHolder::~CTranslationDomainRefHolder() {
    CTranslation::Get().DelReference(m_sDomain);
}
