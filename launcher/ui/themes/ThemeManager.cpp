// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2024 Tayou <git@tayou.org>
 *  Copyright (C) 2023 TheKodeToad <TheKodeToad@proton.me>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, version 3.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#include "ThemeManager.h"

#include <QApplication>
#include <QDir>
#include <QDirListing>
#include <QIcon>
#include <QImageReader>
#include <QStyle>
#include <QStyleFactory>
#include "Result.h"
#include "ui/themes/BrightTheme.h"
#include "ui/themes/CatPack.h"
#include "ui/themes/CustomTheme.h"
#include "ui/themes/DarkTheme.h"
#include "ui/themes/FusionTheme.h"
#include "ui/themes/SystemTheme.h"

#include "Application.h"
#include "settings/SettingsObject.h"

namespace {
class LiquidGlassTheme final : public FusionTheme {
   public:
    QString id() override { return "liquid_glass"; }
    QString name() override { return QObject::tr("Liquid Glass"); }
    QString tooltip() override { return QObject::tr("A deep blue glass theme with cool aqua highlights."); }
    bool hasStyleSheet() override { return true; }
    double fadeAmount() override { return 0.5; }
    QColor fadeColor() override { return QColor(14, 20, 33); }
    QPalette colorScheme() override
    {
        QPalette palette;
        palette.setColor(QPalette::Window, QColor("#101725"));
        palette.setColor(QPalette::WindowText, QColor("#eaf1ff"));
        palette.setColor(QPalette::Base, QColor("#121a29"));
        palette.setColor(QPalette::AlternateBase, QColor("#182338"));
        palette.setColor(QPalette::ToolTipBase, QColor("#263952"));
        palette.setColor(QPalette::ToolTipText, QColor("#f1f7ff"));
        palette.setColor(QPalette::Text, QColor("#eaf1ff"));
        palette.setColor(QPalette::Button, QColor("#202c40"));
        palette.setColor(QPalette::ButtonText, QColor("#edf4ff"));
        palette.setColor(QPalette::BrightText, QColor("#ff8695"));
        palette.setColor(QPalette::Link, QColor("#8de8da"));
        palette.setColor(QPalette::Highlight, QColor("#72dcca"));
        palette.setColor(QPalette::HighlightedText, QColor("#10202b"));
        palette.setColor(QPalette::PlaceholderText, QColor("#9aa9bf"));
        return fadeInactive(palette, fadeAmount(), fadeColor());
    }
    QString appStyleSheet() override
    {
        return R"qss(
            QMainWindow, QDialog, QWizard { background: rgba(14, 21, 35, 105); }
            QWidget { color: #eaf1ff; }
            QWidget#centralWidget { background-color: rgba(16, 23, 37, 85); }
            QMenuBar, QToolBar, QStatusBar { background-color: rgba(25, 36, 55, 150); border: 0; }
            QMenuBar::item { padding: 7px 10px; border-radius: 8px; background: transparent; }
            QMenuBar::item:selected, QMenu::item:selected { background: rgba(112, 220, 203, 48); color: #a3f2e5; }
            QMenu { background: #192438; border: 1px solid #50617a; border-radius: 12px; padding: 6px; }
            QMenu::item { padding: 7px 24px 7px 12px; border-radius: 7px; }
            QToolBar { spacing: 5px; padding: 6px; border-bottom: 1px solid rgba(208, 225, 255, 28); }
            QPushButton, QToolButton { background: qlineargradient(x1:0,y1:0,x2:1,y2:1, stop:0 rgba(112, 151, 184, 150), stop:1 rgba(42, 67, 96, 172)); border: 1px solid rgba(211, 233, 255, 75); border-radius: 11px; padding: 7px 12px; }
            QPushButton:hover, QToolButton:hover { background: rgba(118, 166, 198, 178); border-color: #91eadd; }
            QPushButton:pressed, QToolButton:pressed { background: rgba(60, 108, 126, 190); }
            QPushButton:disabled, QToolButton:disabled { color: #8997ab; border-color: rgba(180, 197, 220, 30); }
            QLineEdit, QTextEdit, QPlainTextEdit, QComboBox, QSpinBox, QDoubleSpinBox { background: rgba(11, 18, 30, 178); border: 1px solid rgba(190, 212, 241, 65); border-radius: 9px; padding: 6px 9px; selection-background-color: #72dcca; selection-color: #10202b; }
            QLineEdit:focus, QTextEdit:focus, QPlainTextEdit:focus, QComboBox:focus, QSpinBox:focus { border-color: #83e6d6; }
            QComboBox::drop-down { border: 0; width: 24px; }
            QTabWidget::pane { border: 1px solid rgba(199, 218, 245, 48); border-radius: 12px; top: -1px; background: rgba(26, 38, 58, 118); }
            QTabBar::tab { background: rgba(22, 32, 49, 150); border: 1px solid rgba(199, 218, 245, 42); border-bottom: 0; border-top-left-radius: 9px; border-top-right-radius: 9px; padding: 7px 12px; margin-right: 3px; }
            QTabBar::tab:selected { background: rgba(65, 97, 121, 178); color: #a7f2e7; }
            QTreeView, QTableView, QListView, QTreeWidget, QTableWidget, QListWidget { background: rgba(14, 21, 34, 112); alternate-background-color: rgba(35, 49, 70, 126); border: 1px solid rgba(192, 213, 241, 42); border-radius: 11px; gridline-color: rgba(202, 219, 243, 35); selection-background-color: rgba(104, 206, 193, 105); selection-color: #f2fbff; }
            QHeaderView::section { background: rgba(33, 48, 70, 185); color: #dce9fb; border: 0; border-bottom: 1px solid rgba(199, 218, 245, 45); padding: 7px; }
            QScrollBar:vertical { background: transparent; width: 12px; margin: 3px; }
            QScrollBar::handle:vertical { background: rgba(156, 183, 213, 100); border-radius: 5px; min-height: 24px; }
            QScrollBar::handle:vertical:hover { background: rgba(141, 232, 218, 180); }
            QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }
            QProgressBar { background: rgba(12, 19, 31, 210); border: 1px solid rgba(190, 212, 241, 54); border-radius: 8px; text-align: center; min-height: 12px; }
            QProgressBar::chunk { background: qlineargradient(x1:0,y1:0,x2:1,y2:0, stop:0 #69cfbf, stop:1 #91c8ff); border-radius: 7px; }
            QCheckBox::indicator, QRadioButton::indicator { width: 15px; height: 15px; border: 1px solid rgba(200, 220, 247, 95); border-radius: 4px; background: rgba(10, 17, 29, 220); }
            QCheckBox::indicator:checked, QRadioButton::indicator:checked { background: #72dcca; border-color: #a4f3e6; }
            QToolTip { color: #f1f7ff; background: #263952; border: 1px solid #7890ac; border-radius: 7px; padding: 5px; }
        )qss";
    }
};
}  // namespace
ThemeManager::ThemeManager()
{
    QIcon::setFallbackThemeName(QIcon::themeName());
    auto searchPaths = QIcon::themeSearchPaths();
    searchPaths.removeOne(":/icons");
    searchPaths.prepend(m_iconThemeFolder.path());
    searchPaths.prepend(":/icons");
    QIcon::setThemeSearchPaths(searchPaths);

    themeDebugLog() << "Determining System Widget Theme...";
    const auto& style = QApplication::style();
    m_defaultStyle = style->objectName();
    themeDebugLog() << "System theme seems to be:" << m_defaultStyle;

    m_defaultPalette = QApplication::palette();

    initializeThemes();
    initializeCatPacks();
}

ThemeManager::~ThemeManager()
{
    stopSettingNewWindowColorsOnMac();
}

/// @brief Adds the Theme to the list of themes
/// @param theme The Theme to add
/// @return Theme ID
QString ThemeManager::addTheme(std::unique_ptr<ITheme> theme)
{
    QString id = theme->id();
    if (!m_themes.contains(id)) {
        m_themes.emplace(id, std::move(theme));
    } else {
        themeWarningLog() << "Theme(" << id << ") not added to prevent id duplication";
    }
    return id;
}

/// @brief Gets the Theme from the List via ID
/// @param themeId Theme ID of theme to fetch
/// @return Theme at themeId
ITheme* ThemeManager::getTheme(const QString& themeId)
{
    return m_themes[themeId].get();
}

QString ThemeManager::addIconTheme(IconTheme theme)
{
    QString id = theme.id();
    if (!m_icons.contains(id)) {
        m_icons.emplace(id, std::move(theme));
    } else {
        themeWarningLog() << "IconTheme(" << id << ") not added to prevent id duplication";
    }
    return id;
}

void ThemeManager::initializeThemes()
{
    // Icon themes
    initializeIcons();

    // Initialize widget themes
    initializeWidgets();
}

void ThemeManager::initializeIcons()
{
    // TODO: icon themes and instance icons do not mesh well together. Rearrange and fix discrepancies!
    // set icon theme search path!
    themeDebugLog() << "<> Initializing Icon Themes";

    for (const QString& id : s_builtinIcons) {
        IconTheme theme(id, QString(":/icons/%1").arg(id));
        if (!theme.load()) {
            themeWarningLog() << "Couldn't load built-in icon theme" << id;
            continue;
        }

        addIconTheme(std::move(theme));
        themeDebugLog() << "Loaded Built-In Icon Theme" << id;
    }

    if (!m_iconThemeFolder.mkpath(".")) {
        themeWarningLog() << "Couldn't create icon theme folder";
    }
    themeDebugLog() << "Icon Theme Folder Path:" << m_iconThemeFolder.absolutePath();

    for (const auto& entry :
         QDirListing(m_iconThemeFolder.path(), QDirListing::IteratorFlag::DirsOnly | QDirListing::IteratorFlag::ResolveSymlinks)) {
        QDir dir(entry.filePath());
        IconTheme theme(dir.dirName(), dir.path());
        if (!theme.load()) {
            continue;
        }

        addIconTheme(std::move(theme));
        themeDebugLog() << "Loaded Custom Icon Theme from" << dir.path();
    }

    themeDebugLog() << "<> Icon themes initialized.";
}

void ThemeManager::initializeWidgets()
{
    themeDebugLog() << "<> Initializing Widget Themes";
    themeDebugLog() << "Loading Built-in Theme:" << addTheme(std::make_unique<SystemTheme>(m_defaultStyle, m_defaultPalette, true));
    auto darkThemeId = addTheme(std::make_unique<DarkTheme>());
    themeDebugLog() << "Loading Built-In Theme:" << addTheme(std::make_unique<LiquidGlassTheme>());
    themeDebugLog() << "Loading Built-in Theme:" << darkThemeId;
    themeDebugLog() << "Loading Built-in Theme:" << addTheme(std::make_unique<BrightTheme>());

    themeDebugLog() << "<> Initializing System Widget Themes";
    QStringList styles = QStyleFactory::keys();
    for (auto& st : styles) {
#ifdef Q_OS_WINDOWS
        if (QSysInfo::productVersion() != "11" && st == "windows11") {
            continue;
        }
#endif
        themeDebugLog() << "Loading System Theme:" << addTheme(std::make_unique<SystemTheme>(st, m_defaultPalette, false));
    }

    // TODO: need some way to differentiate same name themes in different subdirectories
    //  (maybe smaller grey text next to theme name in dropdown?)

    if (!m_applicationThemeFolder.mkpath(".")) {
        themeWarningLog() << "Couldn't create theme folder";
    }
    themeDebugLog() << "Theme Folder Path:" << m_applicationThemeFolder.absolutePath();

    for (const auto& directoryEntry :
         QDirListing(m_applicationThemeFolder.path(), QDirListing::IteratorFlag::DirsOnly | QDirListing::IteratorFlag::ResolveSymlinks)) {
        QDir dir(directoryEntry.filePath());
        QFileInfo themeJson(dir.absoluteFilePath("theme.json"));
        if (themeJson.exists()) {
            // Load "theme.json" based themes
            themeDebugLog() << "Loading JSON Theme from:" << themeJson.absoluteFilePath();
            addTheme(std::make_unique<CustomTheme>(getTheme(darkThemeId), themeJson, true));
        } else {
            // Load pure QSS Themes
            for (const auto& stylesheetEntry :
                 QDirListing(dir.absoluteFilePath(""), { "*.qss", "*.css" },
                             QDirListing::IteratorFlag::FilesOnly | QDirListing::IteratorFlag::ResolveSymlinks)) {
                QFile customThemeFile(stylesheetEntry.absoluteFilePath());
                QFileInfo customThemeFileInfo(customThemeFile);
                themeDebugLog() << "Loading QSS Theme from:" << customThemeFileInfo.absoluteFilePath();
                addTheme(std::make_unique<CustomTheme>(getTheme(darkThemeId), customThemeFileInfo, false));
            }
        }
    }

    themeDebugLog() << "<> Widget themes initialized.";
}

#ifndef Q_OS_MACOS
void ThemeManager::setTitlebarColorOnMac(WId windowId, const QColor& color) {}
void ThemeManager::setTitlebarColorOfAllWindowsOnMac(const QColor& color) {}
void ThemeManager::stopSettingNewWindowColorsOnMac() {}
#endif

QList<IconTheme*> ThemeManager::getValidIconThemes()
{
    QList<IconTheme*> ret;
    ret.reserve(static_cast<qsizetype>(m_icons.size()));
    for (auto&& [id, theme] : m_icons) {
        ret.append(&theme);
    }
    return ret;
}

QList<ITheme*> ThemeManager::getValidApplicationThemes()
{
    QList<ITheme*> ret;
    ret.reserve(static_cast<qsizetype>(m_themes.size()));
    for (auto&& [id, theme] : m_themes) {
        ret.append(theme.get());
    }
    return ret;
}

QList<CatPack*> ThemeManager::getValidCatPacks()
{
    QList<CatPack*> ret;
    ret.reserve(static_cast<qsizetype>(m_catPacks.size()));
    for (auto&& [id, theme] : m_catPacks) {
        ret.append(theme.get());
    }
    return ret;
}

bool ThemeManager::isValidIconTheme(const QString& id)
{
    return !id.isEmpty() && m_icons.contains(id);
}

bool ThemeManager::isValidApplicationTheme(const QString& id)
{
    return !id.isEmpty() && m_themes.contains(id);
}

QDir ThemeManager::getIconThemesFolder()
{
    return m_iconThemeFolder;
}

QDir ThemeManager::getApplicationThemesFolder()
{
    return m_applicationThemeFolder;
}

QDir ThemeManager::getCatPacksFolder()
{
    return m_catPacksFolder;
}

void ThemeManager::setIconTheme(const QString& name)
{
    if (!m_icons.contains(name)) {
        themeWarningLog() << "Tried to set invalid icon theme:" << name;
        return;
    }

    QIcon::setThemeName(name);
}

void ThemeManager::setApplicationTheme(const QString& name, bool initial)
{
    auto systemPalette = qApp->palette();
    auto themeIter = m_themes.find(name);
    if (themeIter != m_themes.end()) {
        auto& theme = themeIter->second;
        themeDebugLog() << "applying theme" << theme->name();
        theme->apply(initial);
        setTitlebarColorOfAllWindowsOnMac(qApp->palette().window().color());

        m_logColors = theme->logColorScheme();
    } else {
        themeWarningLog() << "Tried to set invalid theme:" << name;
    }
}

void ThemeManager::applyCurrentlySelectedTheme(bool initial)
{
    auto* settings = APPLICATION->settings();
    setIconTheme(settings->get("IconTheme").toString());
    themeDebugLog() << "<> Icon theme set.";
    auto applicationTheme = settings->get("ApplicationTheme").toString();
    if (applicationTheme == "") {
        applicationTheme = m_defaultStyle;
    }
    setApplicationTheme(applicationTheme, initial);
    themeDebugLog() << "<> Application theme set.";
}

QString ThemeManager::getCatPack(const QString& catName)
{
    auto catIter = m_catPacks.find(!catName.isEmpty() ? catName : APPLICATION->settings()->get("BackgroundCat").toString());
    if (catIter != m_catPacks.end()) {
        auto& catPack = catIter->second;
        themeDebugLog() << "applying catpack" << catPack->id();
        return catPack->path();
    }
    themeWarningLog() << "Tried to get invalid catPack:" << catName;

    return m_catPacks.begin()->second->path();
}

QString ThemeManager::addCatPack(std::unique_ptr<CatPack> catPack)
{
    QString id = catPack->id();
    if (!m_catPacks.contains(id)) {
        m_catPacks.emplace(id, std::move(catPack));
    } else {
        themeWarningLog() << "CatPack(" << id << ") not added to prevent id duplication";
    }
    return id;
}

void ThemeManager::initializeCatPacks()
{
    QList<std::pair<QString, QString>> defaultCats{ { "kitteh", QObject::tr("Background Cat (from MultiMC)") },
                                                    { "rory", QObject::tr("Rory ID 11 (drawn by Ashtaka)") },
                                                    { "rory-flat", QObject::tr("Rory ID 11 (flat edition, drawn by Ashtaka)") },
                                                    { "teawie", QObject::tr("Teawie (drawn by SympathyTea)") } };
    for (const auto& [id, name] : defaultCats) {
        addCatPack(std::unique_ptr<CatPack>(new BasicCatPack(id, name)));
    }
    if (!m_catPacksFolder.mkpath(".")) {
        themeWarningLog() << "Couldn't create catpacks folder";
    }
    themeDebugLog() << "CatPacks Folder Path:" << m_catPacksFolder.absolutePath();

    QStringList supportedImageFormats;
    for (const auto& format : QImageReader::supportedImageFormats()) {
        supportedImageFormats.append("*." + format);
    }
    auto loadFiles = [this, supportedImageFormats](const QDir& dir) {
        // Load image files directly
        for (const auto& entry : QDirListing(dir.absoluteFilePath(""), supportedImageFormats,
                                             QDirListing::IteratorFlag::FilesOnly | QDirListing::IteratorFlag::ResolveSymlinks)) {
            QFile customCatFile(entry.absoluteFilePath());
            QFileInfo customCatFileInfo(customCatFile);
            themeDebugLog() << "Loading CatPack from:" << customCatFileInfo.absoluteFilePath();
            addCatPack(std::unique_ptr<CatPack>(new FileCatPack(customCatFileInfo)));
        }
    };

    loadFiles(m_catPacksFolder);

    for (const auto& dirEntry :
         QDirListing(m_catPacksFolder.path(), QDirListing::IteratorFlag::DirsOnly | QDirListing::IteratorFlag::ResolveSymlinks)) {
        QDir dir(dirEntry.filePath());
        QFileInfo manifest(dir.absoluteFilePath("catpack.json"));
        if (manifest.isFile()) {
            // Load background manifest
            themeDebugLog() << "Loading background manifest from:" << manifest.absoluteFilePath();
            auto catPack = JsonCatPack::create(manifest);
            if (!catPack) {
                themeWarningLog() << "Couldn't load catpack json:" << catPack.error();
            } else {
                addCatPack(std::move(*catPack));
            }
        } else {
            loadFiles(dir);
        }
    }
}

void ThemeManager::refresh()
{
    m_themes.clear();
    m_icons.clear();
    m_catPacks.clear();

    initializeThemes();
    initializeCatPacks();
}

