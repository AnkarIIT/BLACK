#include <QtTest/QtTest>
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("BLACK");
    
    int result = 0;
    
    // Unit tests
    #include "test_trackerblocker.moc"
    #include "test_vaultcrypto.moc"
    #include "test_shelffstore.moc"
    #include "test_permissionsbridge.moc"
    #include "test_passwordstore.moc"
    #include "test_oauthmanager.moc"
    #include "test_safebrowsing.moc"
    #include "test_tab_thumbnail_download.moc"
    #include "test_browserwindow.moc"
    #include "test_extensionmanager.moc"
    #include "test_account.moc"
    #include "test_downloadmanager.moc"
    #include "test_readermode.moc"
    #include "test_accessibility.moc"
    
    TrackerBlockerFuzzer trackerBlockerTest;
    result |= QTest::qExec(&trackerBlockerTest, argc, argv);
    
    VaultCryptoFuzzer vaultCryptoTest;
    result |= QTest::qExec(&vaultCryptoTest, argc, argv);
    
    SafeBrowsingFuzzer safeBrowsingTest;
    result |= QTest::qExec(&safeBrowsingTest, argc, argv);
    
    return result;
}