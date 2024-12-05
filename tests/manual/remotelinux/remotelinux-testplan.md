# Prerequisites
- The usual set of development tools on the local machine.
- An ssh client on the local machine.
- SSH access to some machine (which might also be the local machine).

Most of the following assumes a Linux host. On Windows, additional prerequisites
and some tweaks to the instructions are necessary.

# Initial environment
Start Qt Creator with clean settings, preferably using the -tcs switch.

# Basic tests

## Create a device
1. Go to `Edit -> Preferences -> Devices`.
2. Click `Add ...`, choose `Remote Linux Device` from the list, and start the wizard.
3. Enter appropriate values for server and user name and proceed to the next page.
  - Simplest is to use `localhost` for the server.
  - You might want to create a dedicated test user for this purpose.
4. Deploy a public key.
  - Removing the key from `~/.ssh/authorized_keys` before/after testing forces you to re-enter
    the password the next time you run this test, which increases the test coverage.
5. Finish the wizard. Qt Creator should now start a device test, which should succeed.
  - If you misconfigured something, fix it and re-run the device test via the `Test` button
    until it succeeds.
6. Check that the `Current state` field has the value `Unknown`.
7. Press the `Apply` button.
8. On the right side of the settings widget, click `Show Running Processes ...` and
   `Open Remote Shell` and check that they do what they are supposed to.

## Create a Kit for running
1. Go to `Edit -> Preferences -> Kits`.
2. Create a new kit and set `Run Device` to `Remote Linux Device`. The device created
   in the previous step should get chosen automatically in the `Device` combo box.
3. Make sure that the kit's toolchain is compatible with the device.
4. Press the "OK" button.

## Deploy and run an application
1. Create a simple app via `File -> New Project -> Non-Qt Project -> Simple C Application`.
  - Make sure you select the newly created kit in the last step.
2. Edit the project file such that the application will get deployed to an accessible
   location on the target machine.
  - For Qmake: Add `target.path = /tmp/mytest` and `INSTALLS += target`.
  - For qbs: Add `qbs.installPrefix: "/tmp/mytest"`.
  - For CMake and other exotic build tools, check the respective documentation.
3. Click the `Run` button. The project should get built and deployed, and the output
   "Hello World" should appear in the application output pane.

# Advanced tests

## Try to deploy and run an application on a "broken" device
1. "Break" your device by pointing it to an invalid host name in the settings,
   e.g. `1.2.3.4`.
  - **Do not run the device test!**
  - For extra test coverage, repeat this entire scenario with an invalid user
    instead of an invalid host.
2. Press `OK`.
3. Click the `Run` button. Expected results:
  - Deployment should fail and create an issue in the Issues pane. This may take some
    time, and Qt Creator might appear frozen temporarily.
  - A note should pop up informing you that the device is now marked as "disonnected".
4. Go back to the device settings.
  - Verify that the device is now marked with a red dot and `Current state` is `Disconnected`.
  - Revert the changes from step 1. **Again, do not run the device test.**
5. Click the `Run` button. This should fail again, but faster and without blocking.
6. Go to the device settings again.
  - Re-run the device test and verify that is succeeds.
  - The red dot should have disappeared and the state should be back to `Unknown`.
  - Press `OK`.
7. Click the `Run` button. Deployment should work again, and "Hello World" should appear
   in the application output pane.

## Create a kit for building
1. Go to `Edit -> Preferences -> Kits`.
2. Create a new kit and set `Build Device` to `Remote Linux` device. The device created
   in the previous step should get chosen automatically in the `Device` combo box.
3. Press the "OK" button.

## Create a remote project
1. Create a simple app via `File -> New Project -> Non-Qt Project -> Simple C Application`.
  - Make sure you choose a **remote path** on the first page
  - Make sure you select the newly created kit in the last step.
2. Check that Qt Creator at least tries to load the project.
  - This operation might be exceedingly slow.
  - It is also possible that the build tool will error out.
  - What is important is that some semblance of a project tree appears, with the project
    file pointing to a remote path, and there is no device-related error message.

## Open a project on a "broken" device
1. "Break" your device as described above.
2. Re-open your last project, e.g. via `File->Recent Projects`.
3. This should fail, possibly after some time, and a note about the device being
   disconnected should appear.
4. Close the project.
5. "Fix" the device again as in the "deploy and run" scenario, once with and once
   without re-running the device test, and again observe that the operation fails again
   and succeeds, respectively.
