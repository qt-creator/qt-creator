QtKeychain
==========

QtKeychain is a Qt API to store passwords and other secret data securely. How the data is stored depends on the platform:

 * **macOS:** Passwords are stored in the macOS Keychain.

 * **Linux/Unix:** If running, GNOME Keyring is used, otherwise QtKeychain tries to use KWallet (via D-Bus), if available. Libsecret (common API for desktop-specific solutions)
   is also supported.

 * **Windows:** By default, the Windows Credential Store is used (requires Windows 7 or newer).
Pass `-DUSE_CREDENTIAL_STORE=OFF` to cmake to disable it. If disabled, QtKeychain uses the Windows API function
[CryptProtectData](http://msdn.microsoft.com/en-us/library/windows/desktop/aa380261%28v=vs.85%29.aspx "CryptProtectData function")
to encrypt the password with the user's logon credentials. The encrypted data is then persisted via QSettings.

 * **Android and iOS:** Passwords are stored in the Android keystore system and iOS keychain, respectively.

In unsupported environments QtKeychain will report an error. It will not store any data unencrypted unless explicitly requested (`setInsecureFallback( true )`).


Requirements
------------

QtKeychain 0.12 and newer supports Qt 5 and Qt 6 and requires a compiler with C++11 support. Older versions support Qt 4 and Qt 5.

License
-------

QtKeychain is available under the [Modified BSD License](http://www.gnu.org/licenses/license-list.html#ModifiedBSD). See the file COPYING for details.
