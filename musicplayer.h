#pragma once

#include <QObject>
#include <QMediaPlayer>

class QAudioOutput;
class QListWidgetItem;
class QPropertyAnimation;

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MusicPlayer : public QObject
{
private:
    Q_OBJECT

    using super = QObject;

public:
    explicit MusicPlayer(Ui::MainWindow* mainWindowUI, QObject *parent = nullptr);

private slots:
    void OnPlayMusicButtonClicked();
    void OnPauseMusicButtonClicked();
    void OnStopMusicButtonClicked();
    void OnRemoveFromPlaylistClicked();
    void OnAddToPlaylistButtonClicked();
    void OnVerticalSliderValueChanged(int value);
    void OnMusicFadeSliderValueChanged(int value);
    void MusicCursorChanged(qint64);
    void MediaStatusChanged(QMediaPlayer::MediaStatus status);
    void OnHorizontalSliderSliderReleased();
    void OnPreviousMusicButtonClicked();
    void OnNextMusicButtonClicked();
    void OnMusicPlaylistItemDoubleClicked(QListWidgetItem* item);
    void OnMusicPlaylistItemPressed(QListWidgetItem* item);
    void SectionItemClicked(const QString& categoryName, const QString& itemName);
    void SectionItemDoubleClicked(const QString& categoryName, const QString& itemName);

private:
    void PlayMusic(int playlistRow);
    void SetAudioSource(const QUrl& source);
    void OnMusicRowChanged();
    void PlayNextMusic(bool ignoreRepeat = false);
    void PlayPreviousMusic(bool ignoreRepeat = false);

    void AddToPlaylist(QListWidgetItem* item);
    void RemoveFromPlaylist();

    bool IsInPlaylist(QListWidgetItem* item) const;
    bool IsInPlaylist(const QString& itemName) const;

    void PlaylistChanged();
    void SelectPlayingMusic();
    void UnselectAllInPlaylist();

    void FadeOut();
    float GetCurrentVolume() const;
    float GetFadeDuration() const;

private:
    Ui::MainWindow *ui { nullptr };

    QMediaPlayer* m_Player { nullptr };
    QAudioOutput* m_AudioOutput { nullptr };

    QPropertyAnimation* m_FadeOutAnimation { nullptr };
    QMediaPlayer* m_FadeOutPlayer { nullptr };
    QAudioOutput* m_FadeOutAudioOutput { nullptr };

    QString m_CurrentAudioName;
    int m_CurrentAudioRow { 0 };
    bool m_IsAudioSourceBeingSet { false };

    QStringList m_PlaylistItemList;
};
