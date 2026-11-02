#include "ui/error_box.hpp"
#include "ui/nvg_util.hpp"
#include "app.hpp"
#include "i18n.hpp"

namespace sphaira::ui {
namespace {

auto GetModule(Result rc) -> const char* {
    switch (R_MODULE(rc)) {
        case Module_Svc: return "Svc";
        case Module_Fs: return "Fs";
        case Module_Os: return "Os";
        case Module_Ncm: return "Ncm";
        case Module_Ns: return "Ns";
        case Module_Spl: return "Spl";
        case Module_Applet: return "Applet";
        case Module_Usb: return "Usb";
        case Module_Irsensor: return "Irsensor";
        case Module_Libnx: return "Libnx";
        case Module_Sphaira: return "NinMagicFoil";
    }

    return nullptr;
}
auto GetCodeMessage(Result rc) -> const char* {
    switch (rc) {
        case SvcError_TimedOut: return "SvcError_TimedOut";
        case SvcError_Cancelled: return "SvcError_Cancelled";

        case FsError_PathNotFound: return "FsError_PathNotFound";
        case FsError_PathAlreadyExists: return "FsError_PathAlreadyExists";
        case FsError_TargetLocked: return "FsError_TargetLocked";
        case FsError_TooLongPath: return "FsError_TooLongPath";
        case FsError_InvalidCharacter: return "FsError_InvalidCharacter";
        case FsError_InvalidOffset: return "FsError_InvalidOffset";
        case FsError_InvalidSize: return "FsError_InvalidSize";

        case Result_TransferCancelled: return "NinMagicFoilError_TransferCancelled";
        case Result_StreamBadSeek: return "NinMagicFoilError_StreamBadSeek";
        case Result_FsTooManyEntries: return "NinMagicFoilError_FsTooManyEntries";
        case Result_FsNewPathTooLarge: return "NinMagicFoilError_FsNewPathTooLarge";
        case Result_FsInvalidType: return "NinMagicFoilError_FsInvalidType";
        case Result_FsEmpty: return "NinMagicFoilError_FsEmpty";
        case Result_FsAlreadyRoot: return "NinMagicFoilError_FsAlreadyRoot";
        case Result_FsNoCurrentPath: return "NinMagicFoilError_FsNoCurrentPath";
        case Result_FsBrokenCurrentPath: return "NinMagicFoilError_FsBrokenCurrentPath";
        case Result_FsIndexOutOfBounds: return "NinMagicFoilError_FsIndexOutOfBounds";
        case Result_FsFsNotActive: return "NinMagicFoilError_FsFsNotActive";
        case Result_FsNewPathEmpty: return "NinMagicFoilError_FsNewPathEmpty";
        case Result_FsLoadingCancelled: return "NinMagicFoilError_FsLoadingCancelled";
        case Result_FsBrokenRoot: return "NinMagicFoilError_FsBrokenRoot";
        case Result_FsUnknownStdioError: return "NinMagicFoilError_FsUnknownStdioError";
        case Result_FsStdioFailedToSeek: return "NinMagicFoilError_FsStdioFailedToSeek";
        case Result_FsStdioFailedToRead: return "NinMagicFoilError_FsStdioFailedToRead";
        case Result_FsStdioFailedToWrite: return "NinMagicFoilError_FsStdioFailedToWrite";
        case Result_FsStdioFailedToOpenFile: return "NinMagicFoilError_FsStdioFailedToOpenFile";
        case Result_FsStdioFailedToCreate: return "NinMagicFoilError_FsStdioFailedToCreate";
        case Result_FsStdioFailedToTruncate: return "NinMagicFoilError_FsStdioFailedToTruncate";
        case Result_FsStdioFailedToFlush: return "NinMagicFoilError_FsStdioFailedToFlush";
        case Result_FsStdioFailedToCreateDirectory: return "NinMagicFoilError_FsStdioFailedToCreateDirectory";
        case Result_FsStdioFailedToDeleteFile: return "NinMagicFoilError_FsStdioFailedToDeleteFile";
        case Result_FsStdioFailedToDeleteDirectory: return "NinMagicFoilError_FsStdioFailedToDeleteDirectory";
        case Result_FsStdioFailedToOpenDirectory: return "NinMagicFoilError_FsStdioFailedToOpenDirectory";
        case Result_FsStdioFailedToRename: return "NinMagicFoilError_FsStdioFailedToRename";
        case Result_FsStdioFailedToStat: return "NinMagicFoilError_FsStdioFailedToStat";
        case Result_FsReadOnly: return "NinMagicFoilError_FsReadOnly";
        case Result_FsNotActive: return "NinMagicFoilError_FsNotActive";
        case Result_FsFailedStdioStat: return "NinMagicFoilError_FsFailedStdioStat";
        case Result_FsFailedStdioOpendir: return "NinMagicFoilError_FsFailedStdioOpendir";
        case Result_NroBadMagic: return "NinMagicFoilError_NroBadMagic";
        case Result_NroBadSize: return "NinMagicFoilError_NroBadSize";
        case Result_AppFailedMusicDownload: return "NinMagicFoilError_AppFailedMusicDownload";
        case Result_CurlFailedEasyInit: return "NinMagicFoilError_CurlFailedEasyInit";
        case Result_DumpFailedNetworkUpload: return "NinMagicFoilError_DumpFailedNetworkUpload";
        case Result_UnzOpen2_64: return "NinMagicFoilError_UnzOpen2_64";
        case Result_UnzGetGlobalInfo64: return "NinMagicFoilError_UnzGetGlobalInfo64";
        case Result_UnzLocateFile: return "NinMagicFoilError_UnzLocateFile";
        case Result_UnzGoToFirstFile: return "NinMagicFoilError_UnzGoToFirstFile";
        case Result_UnzGoToNextFile: return "NinMagicFoilError_UnzGoToNextFile";
        case Result_UnzOpenCurrentFile: return "NinMagicFoilError_UnzOpenCurrentFile";
        case Result_UnzGetCurrentFileInfo64: return "NinMagicFoilError_UnzGetCurrentFileInfo64";
        case Result_UnzReadCurrentFile: return "NinMagicFoilError_UnzReadCurrentFile";
        case Result_ZipOpen2_64: return "NinMagicFoilError_ZipOpen2_64";
        case Result_ZipOpenNewFileInZip: return "NinMagicFoilError_ZipOpenNewFileInZip";
        case Result_ZipWriteInFileInZip: return "NinMagicFoilError_ZipWriteInFileInZip";
        case Result_MmzBadLocalHeaderSig: return "NinMagicFoilError_MmzBadLocalHeaderSig";
        case Result_MmzBadLocalHeaderRead: return "NinMagicFoilError_MmzBadLocalHeaderRead";
        case Result_FileBrowserFailedUpload: return "NinMagicFoilError_FileBrowserFailedUpload";
        case Result_FileBrowserDirNotDaybreak: return "NinMagicFoilError_FileBrowserDirNotDaybreak";
        case Result_AppstoreFailedZipDownload: return "NinMagicFoilError_AppstoreFailedZipDownload";
        case Result_AppstoreFailedMd5: return "NinMagicFoilError_AppstoreFailedMd5";
        case Result_AppstoreFailedParseManifest: return "NinMagicFoilError_AppstoreFailedParseManifest";
        case Result_GameBadReadForDump: return "NinMagicFoilError_GameBadReadForDump";
        case Result_GameEmptyMetaEntries: return "NinMagicFoilError_GameEmptyMetaEntries";
        case Result_GameMultipleKeysFound: return "NinMagicFoilError_GameMultipleKeysFound";
        case Result_GameNoNspEntriesBuilt: return "NinMagicFoilError_GameNoNspEntriesBuilt";
        case Result_KeyMissingNcaKeyArea: return "NinMagicFoilError_KeyMissingNcaKeyArea";
        case Result_KeyMissingTitleKek: return "NinMagicFoilError_KeyMissingTitleKek";
        case Result_KeyMissingMasterKey: return "NinMagicFoilError_KeyMissingMasterKey";
        case Result_KeyFailedDecyptETicketDeviceKey: return "NinMagicFoilError_KeyFailedDecyptETicketDeviceKey";
        case Result_NcaFailedNcaHeaderHashVerify: return "NinMagicFoilError_NcaFailedNcaHeaderHashVerify";
        case Result_NcaBadSigKeyGen: return "NinMagicFoilError_NcaBadSigKeyGen";
        case Result_GcBadReadForDump: return "NinMagicFoilError_GcBadReadForDump";
        case Result_GcEmptyGamecard: return "NinMagicFoilError_GcEmptyGamecard";
        case Result_GcBadXciMagic: return "NinMagicFoilError_GcBadXciMagic";
        case Result_GcBadXciRomSize: return "NinMagicFoilError_GcBadXciRomSize";
        case Result_GcFailedToGetSecurityInfo: return "NinMagicFoilError_GcFailedToGetSecurityInfo";
        case Result_GhdlEmptyAsset: return "NinMagicFoilError_GhdlEmptyAsset";
        case Result_GhdlFailedToDownloadAsset: return "NinMagicFoilError_GhdlFailedToDownloadAsset";
        case Result_GhdlFailedToDownloadAssetJson: return "NinMagicFoilError_GhdlFailedToDownloadAssetJson";
        case Result_GhdlUnsafeArchivePath: return "NinMagicFoilError_GhdlUnsafeArchivePath";
        case Result_ThemezerFailedToDownloadThemeMeta: return "NinMagicFoilError_ThemezerFailedToDownloadThemeMeta";
        case Result_ThemezerFailedToDownloadTheme: return "NinMagicFoilError_ThemezerFailedToDownloadTheme";
        case Result_MainFailedToDownloadUpdate: return "NinMagicFoilError_MainFailedToDownloadUpdate";
        case Result_MainDownloadedUpdateInvalid: return "NinMagicFoilError_MainDownloadedUpdateInvalid";
        case Result_MainFailedToInstallUpdate: return "NinMagicFoilError_MainFailedToInstallUpdate";
        case Result_NxVersionsFailedToDownload: return "NinMagicFoilError_NxVersionsFailedToDownload";
        case Result_NxVersionsInvalidDatabase: return "NinMagicFoilError_NxVersionsInvalidDatabase";
        case Result_UsbDsBadDeviceSpeed: return "NinMagicFoilError_UsbDsBadDeviceSpeed";
        case Result_NcaBadMagic: return "NinMagicFoilError_NcaBadMagic";
        case Result_NspBadMagic: return "NinMagicFoilError_NspBadMagic";
        case Result_XciBadMagic: return "NinMagicFoilError_XciBadMagic";
        case Result_XciSecurePartitionNotFound: return "NinMagicFoilError_XciSecurePartitionNotFound";
        case Result_EsBadTitleKeyType: return "NinMagicFoilError_EsBadTitleKeyType";
        case Result_EsPersonalisedTicketDeviceIdMissmatch: return "NinMagicFoilError_EsPersonalisedTicketDeviceIdMissmatch";
        case Result_EsFailedDecryptPersonalisedTicket: return "NinMagicFoilError_EsFailedDecryptPersonalisedTicket";
        case Result_EsBadDecryptedPersonalisedTicketSize: return "NinMagicFoilError_EsBadDecryptedPersonalisedTicketSize";
        case Result_EsInvalidTicketBadRightsId: return "NinMagicFoilError_EsInvalidTicketBadRightsId";
        case Result_EsInvalidTicketFromatVersion: return "NinMagicFoilError_EsInvalidTicketFromatVersion";
        case Result_EsInvalidTicketKeyType: return "NinMagicFoilError_EsInvalidTicketKeyType";
        case Result_EsInvalidTicketKeyRevision: return "NinMagicFoilError_EsInvalidTicketKeyRevision";
        case Result_OwoBadArgs: return "NinMagicFoilError_OwoBadArgs";
        case Result_UsbCancelled: return "NinMagicFoilError_UsbCancelled";
        case Result_UsbBadMagic: return "NinMagicFoilError_UsbBadMagic";
        case Result_UsbBadVersion: return "NinMagicFoilError_UsbBadVersion";
        case Result_UsbBadCount: return "NinMagicFoilError_UsbBadCount";
        case Result_UsbBadBufferAlign: return "NinMagicFoilError_UsbBadBufferAlign";
        case Result_UsbBadTransferSize: return "NinMagicFoilError_UsbBadTransferSize";
        case Result_UsbEmptyTransferSize: return "NinMagicFoilError_UsbEmptyTransferSize";
        case Result_UsbOverflowTransferSize: return "NinMagicFoilError_UsbOverflowTransferSize";
        case Result_UsbUploadBadMagic: return "NinMagicFoilError_UsbUploadBadMagic";
        case Result_UsbUploadExit: return "NinMagicFoilError_UsbUploadExit";
        case Result_UsbUploadBadCount: return "NinMagicFoilError_UsbUploadBadCount";
        case Result_UsbUploadBadTransferSize: return "NinMagicFoilError_UsbUploadBadTransferSize";
        case Result_UsbUploadBadTotalSize: return "NinMagicFoilError_UsbUploadBadTotalSize";
        case Result_UsbUploadBadCommand: return "NinMagicFoilError_UsbUploadBadCommand";
        case Result_YatiContainerNotFound: return "NinMagicFoilError_YatiContainerNotFound";
        case Result_YatiNcaNotFound: return "NinMagicFoilError_YatiNcaNotFound";
        case Result_YatiInvalidNcaReadSize: return "NinMagicFoilError_YatiInvalidNcaReadSize";
        case Result_YatiInvalidNcaSigKeyGen: return "NinMagicFoilError_YatiInvalidNcaSigKeyGen";
        case Result_YatiInvalidNcaMagic: return "NinMagicFoilError_YatiInvalidNcaMagic";
        case Result_YatiInvalidNcaSignature0: return "NinMagicFoilError_YatiInvalidNcaSignature0";
        case Result_YatiInvalidNcaSignature1: return "NinMagicFoilError_YatiInvalidNcaSignature1";
        case Result_YatiInvalidNcaSha256: return "NinMagicFoilError_YatiInvalidNcaSha256";
        case Result_YatiNczSectionNotFound: return "NinMagicFoilError_YatiNczSectionNotFound";
        case Result_YatiInvalidNczSectionCount: return "NinMagicFoilError_YatiInvalidNczSectionCount";
        case Result_YatiNczBlockNotFound: return "NinMagicFoilError_YatiNczBlockNotFound";
        case Result_YatiInvalidNczBlockVersion: return "NinMagicFoilError_YatiInvalidNczBlockVersion";
        case Result_YatiInvalidNczBlockType: return "NinMagicFoilError_YatiInvalidNczBlockType";
        case Result_YatiInvalidNczBlockTotal: return "NinMagicFoilError_YatiInvalidNczBlockTotal";
        case Result_YatiInvalidNczBlockSizeExponent: return "NinMagicFoilError_YatiInvalidNczBlockSizeExponent";
        case Result_YatiInvalidNczZstdError: return "NinMagicFoilError_YatiInvalidNczZstdError";
        case Result_YatiTicketNotFound: return "NinMagicFoilError_YatiTicketNotFound";
        case Result_YatiInvalidTicketBadRightsId: return "NinMagicFoilError_YatiInvalidTicketBadRightsId";
        case Result_YatiCertNotFound: return "NinMagicFoilError_YatiCertNotFound";
        case Result_YatiNcmDbCorruptHeader: return "NinMagicFoilError_YatiNcmDbCorruptHeader";
        case Result_YatiNcmDbCorruptInfos: return "NinMagicFoilError_YatiNcmDbCorruptInfos";

        case Result_NszFailedCreateCctx: return "NinMagicFoilError_NszFailedCreateCctx";
        case Result_NszFailedSetCompressionLevel: return "NinMagicFoilError_NszFailedSetCompressionLevel";
        case Result_NszFailedSetThreadCount: return "NinMagicFoilError_NszFailedSetThreadCount";
        case Result_NszFailedSetLongDistanceMode: return "NinMagicFoilError_NszFailedSetLongDistanceMode";
        case Result_NszFailedResetCctx: return "NinMagicFoilError_NszFailedResetCctx";
        case Result_NszFailedCompress2: return "NinMagicFoilError_NszFailedCompress2";
        case Result_NszFailedCompressStream2: return "NinMagicFoilError_NszFailedCompressStream2";
        case Result_NszTooManyBlocks: return "NinMagicFoilError_NszTooManyBlocks";
        case Result_NszMissingBlocks: return "NinMagicFoilError_NszMissingBlocks";
        case Result_NspInvalidHeader: return "NinMagicFoilError_NspInvalidHeader";
        case Result_MspMissingManifest: return "NinMagicFoilError_MspMissingManifest";
        case Result_MspInvalidManifest: return "NinMagicFoilError_MspInvalidManifest";
        case Result_MspInvalidTitleId: return "NinMagicFoilError_MspInvalidTitleId";
        case Result_MspInvalidVersion: return "NinMagicFoilError_MspInvalidVersion";
        case Result_MspInvalidPatchset: return "NinMagicFoilError_MspInvalidPatchset";
        case Result_MspInvalidEntry: return "NinMagicFoilError_MspInvalidEntry";
        case Result_MspDuplicateEntry: return "NinMagicFoilError_MspDuplicateEntry";
        case Result_MspNoPayload: return "NinMagicFoilError_MspNoPayload";
        case Result_YatiHttpReadFailed: return "NinMagicFoilError_YatiHttpReadFailed";
        case Result_UsbBadPacketCrc: return "NinMagicFoilError_UsbBadPacketCrc";
        case Result_UsbBadDataCrc: return "NinMagicFoilError_UsbBadDataCrc";
        case Result_UsbRemoteError: return "NinMagicFoilError_UsbRemoteError";
        case Result_SaveInvalidArchiveLayout: return "NinMagicFoilError_SaveInvalidArchiveLayout";
    }

    return "";
}

// short, actionable advice for errors whose cause is known.
// only add codes here where the cause is confirmed, a wrong hint is worse than none.
auto GetCodeHint(Result rc) -> std::string {
    switch (rc) {
        case FsError_UsableSpaceNotEnoughSdCard:
        case FsError_UsableSpaceNotEnoughMmcUser:
        case FsError_UsableSpaceNotEnoughMmcSystem:
            return "Not enough free space on the install location. Free up space or change the install location."_i18n;

        case Result_KeyMissingNcaKeyArea:
        case Result_KeyMissingTitleKek:
        case Result_KeyMissingMasterKey:
        case Result_KeyFailedDecyptETicketDeviceKey:
            return "Your keys are missing or outdated. Dump prod.keys again with the latest Lockpick_RCM."_i18n;

        case Result_YatiInvalidNczZstdError:
            return "Failed to decompress the NCZ. Try installing from the SD card or use the USB install script."_i18n;

        case Result_GcEmptyGamecard:
            return "No game card was found. Make sure it is inserted correctly."_i18n;

        case FsError_TargetLocked:
            return "The file or folder is in use. Close any running software, turn off FTP / MTP, or restart the console."_i18n;
    }

    return {};
}

} // namespace

ErrorBox::ErrorBox(const std::string& message) : m_message{message} {
    log_write("[ERROR] %s\n", m_message.c_str());

    m_pos.w = 770.f;
    m_pos.h = 430.f;
    m_pos.x = 255;
    m_pos.y = 145;

    SetAction(Button::A, Action{[this](){
        SetPop();
    }});

    App::PlaySoundEffect(SoundEffect::Error);
}

ErrorBox::ErrorBox(Result code, const std::string& message) : ErrorBox{message} {
    m_code = code;
    m_code_message = GetCodeMessage(code);
    m_hint = GetCodeHint(code);
    m_code_module = std::to_string(R_MODULE(code));
    if (auto str = GetModule(code)) {
        m_code_module += " (" + std::string(str) + ")";
    }
    log_write("[ERROR] Code: 0x%X Module: %s Description: %u\n", R_VALUE(code), m_code_module.c_str(), R_DESCRIPTION(code));
}

ErrorBox::ErrorBox(Result code, const std::string& message, const std::string& detail) : ErrorBox{code, message} {
    if (!detail.empty()) {
        m_hint += (m_hint.empty() ? "" : "\n") + detail;
    }
}

auto ErrorBox::Update(Controller* controller, TouchInfo* touch) -> void {
    Widget::Update(controller, touch);
}

auto ErrorBox::Draw(NVGcontext* vg, Theme* theme) -> void {
    gfx::dimBackground(vg);
    gfx::drawRect(vg, m_pos, theme->GetColour(ThemeEntryID_POPUP));

    const Vec4 box = { 455, 470, 365, 65 };
    const auto center_x = m_pos.x + m_pos.w/2;

    gfx::drawTextArgs(vg, center_x, 180, 63, NVG_ALIGN_CENTER | NVG_ALIGN_TOP, theme->GetColour(ThemeEntryID_ERROR), "\uE140");
    if (m_code.has_value()) {
        const auto code = m_code.value();
        if (m_code_message.empty()) {
            gfx::drawTextArgs(vg, center_x, 270, 25, NVG_ALIGN_CENTER | NVG_ALIGN_TOP, theme->GetColour(ThemeEntryID_TEXT), "Code: 0x%X Module: %s", R_VALUE(code), m_code_module.c_str());
        } else {
            gfx::drawTextArgs(vg, center_x, 270, 25, NVG_ALIGN_CENTER | NVG_ALIGN_TOP, theme->GetColour(ThemeEntryID_TEXT), "%s", m_code_message.c_str());
        }
    } else {
        gfx::drawTextArgs(vg, center_x, 270, 25, NVG_ALIGN_CENTER | NVG_ALIGN_TOP, theme->GetColour(ThemeEntryID_TEXT), "An error occurred"_i18n.c_str());
    }
    const bool has_hint = !m_hint.empty();
    gfx::drawTextArgs(vg, center_x, has_hint ? 305 : 325, 23, NVG_ALIGN_CENTER | NVG_ALIGN_TOP, theme->GetColour(ThemeEntryID_TEXT), "%s", m_message.c_str());
    if (has_hint) {
        gfx::drawTextBox(vg, m_pos.x + 35, 340, 20, m_pos.w - 70, theme->GetColour(ThemeEntryID_TEXT_SELECTED), m_hint.c_str(), NVG_ALIGN_CENTER | NVG_ALIGN_TOP);
    }
    gfx::drawTextArgs(vg, center_x, has_hint ? 395 : 380, 20, NVG_ALIGN_CENTER | NVG_ALIGN_TOP, theme->GetColour(ThemeEntryID_TEXT_INFO), "If this message appears repeatedly, please open an issue."_i18n.c_str());
    gfx::drawTextArgs(vg, center_x, has_hint ? 425 : 415, 20, NVG_ALIGN_CENTER | NVG_ALIGN_TOP, theme->GetColour(ThemeEntryID_TEXT_INFO), "https://github.com/ITotalJustice/sphaira/issues");
    gfx::drawRectOutline(vg, theme, 4.f, box);
    gfx::drawTextArgs(vg, center_x, box.y + box.h/2, 23, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE, theme->GetColour(ThemeEntryID_TEXT_SELECTED), "OK"_i18n.c_str());
}

} // namespace sphaira::ui
