#pragma once

#include <QObject>

// Central visual scale for Ultimate HAI. Colors and font families stay live
// from the Omarchy theme; geometry, type, spacing and motion live here so the
// whole application shares one rhythm.
class DesignTokens : public QObject {
    Q_OBJECT
    Q_PROPERTY(int space1 READ space1 CONSTANT)
    Q_PROPERTY(int space2 READ space2 CONSTANT)
    Q_PROPERTY(int space3 READ space3 CONSTANT)
    Q_PROPERTY(int space4 READ space4 CONSTANT)
    Q_PROPERTY(int space5 READ space5 CONSTANT)
    Q_PROPERTY(int space6 READ space6 CONSTANT)
    Q_PROPERTY(int space7 READ space7 CONSTANT)
    Q_PROPERTY(int space8 READ space8 CONSTANT)
    Q_PROPERTY(int space9 READ space9 CONSTANT)
    Q_PROPERTY(int radiusSm READ radiusSm CONSTANT)
    Q_PROPERTY(int radiusMd READ radiusMd CONSTANT)
    Q_PROPERTY(int radiusLg READ radiusLg CONSTANT)
    Q_PROPERTY(int radiusPill READ radiusPill CONSTANT)
    Q_PROPERTY(double textMicro READ textMicro CONSTANT)
    Q_PROPERTY(double textCaption READ textCaption CONSTANT)
    Q_PROPERTY(double textLabel READ textLabel CONSTANT)
    Q_PROPERTY(double textBody READ textBody CONSTANT)
    Q_PROPERTY(double textBodyLarge READ textBodyLarge CONSTANT)
    Q_PROPERTY(double textTitle READ textTitle CONSTANT)
    Q_PROPERTY(double textHeadline READ textHeadline CONSTANT)
    Q_PROPERTY(double textDisplay READ textDisplay CONSTANT)
    Q_PROPERTY(int sidebarWidth READ sidebarWidth CONSTANT)
    Q_PROPERTY(int sidebarCompactWidth READ sidebarCompactWidth CONSTANT)
    Q_PROPERTY(int headerHeight READ headerHeight CONSTANT)
    Q_PROPERTY(int statusHeight READ statusHeight CONSTANT)
    Q_PROPERTY(int contentMax READ contentMax CONSTANT)
    Q_PROPERTY(int controlHeight READ controlHeight CONSTANT)
    Q_PROPERTY(int controlHeightSm READ controlHeightSm CONSTANT)
    Q_PROPERTY(int iconSm READ iconSm CONSTANT)
    Q_PROPERTY(int iconMd READ iconMd CONSTANT)
    Q_PROPERTY(int iconLg READ iconLg CONSTANT)
    Q_PROPERTY(int durationFast READ durationFast CONSTANT)
    Q_PROPERTY(int durationNormal READ durationNormal CONSTANT)

public:
    explicit DesignTokens(QObject *parent = nullptr) : QObject(parent) {}

    int space1() const { return 4; }
    int space2() const { return 8; }
    int space3() const { return 12; }
    int space4() const { return 16; }
    int space5() const { return 20; }
    int space6() const { return 24; }
    int space7() const { return 32; }
    int space8() const { return 40; }
    int space9() const { return 48; }
    int radiusSm() const { return 6; }
    int radiusMd() const { return 10; }
    int radiusLg() const { return 16; }
    int radiusPill() const { return 999; }
    qreal textMicro() const { return 9.5; }
    qreal textCaption() const { return 10.5; }
    qreal textLabel() const { return 11.5; }
    qreal textBody() const { return 13.0; }
    qreal textBodyLarge() const { return 15.0; }
    qreal textTitle() const { return 19.0; }
    qreal textHeadline() const { return 25.0; }
    qreal textDisplay() const { return 32.0; }
    int sidebarWidth() const { return 236; }
    int sidebarCompactWidth() const { return 68; }
    int headerHeight() const { return 60; }
    int statusHeight() const { return 30; }
    int contentMax() const { return 1180; }
    int controlHeight() const { return 36; }
    int controlHeightSm() const { return 28; }
    int iconSm() const { return 14; }
    int iconMd() const { return 18; }
    int iconLg() const { return 22; }
    int durationFast() const { return 120; }
    int durationNormal() const { return 180; }
};
