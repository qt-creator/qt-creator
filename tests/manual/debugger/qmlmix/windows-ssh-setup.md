# Windows SSH access for remote CDB / MCP work

To continue the CDB native combined work without shipping a machine back and
forth, a colleague on Windows can expose their box over SSH. We then get a
shell (build `qtcreatorcdbext`, run raw `cdb`) and can tunnel Qt Creator's MCP
port to drive/observe a session remotely.

The Windows side only runs an SSH **server**. The MCP port-forwarding is done
from the connecting side, so no extra Windows configuration is needed for it.

## On the Windows machine (PowerShell as Administrator)

```powershell
# 1. Install the OpenSSH Server feature
Add-WindowsCapability -Online -Name OpenSSH.Server~~~~0.0.1.0

# 2. Start it now, and on every boot
Start-Service sshd
Set-Service -Name sshd -StartupType Automatic

# 3. Make sure the inbound firewall rule exists (installer usually adds it)
if (-not (Get-NetFirewallRule -Name 'OpenSSH-Server-In-TCP' -ErrorAction SilentlyContinue)) {
    New-NetFirewallRule -Name sshd -DisplayName 'OpenSSH Server (sshd)' `
        -Enabled True -Direction Inbound -Protocol TCP -Action Allow -LocalPort 22
}

# 4. (recommended) default to PowerShell instead of cmd over SSH
New-ItemProperty -Path "HKLM:\SOFTWARE\OpenSSH" -Name DefaultShell `
    -Value "C:\Windows\System32\WindowsPowerShell\v1.0\powershell.exe" `
    -PropertyType String -Force
```

## Key-based auth (recommended - avoids passwords)

The connecting person generates a keypair and shares the *public* key:

```bash
ssh-keygen -t ed25519        # then share the .pub contents
```

The Windows user installs that public key. Where depends on the account:

```powershell
# Non-admin account:
mkdir $env:USERPROFILE\.ssh -Force
# paste the public-key line into:  $env:USERPROFILE\.ssh\authorized_keys

# Admin account (Windows quirk - keys live in a system file):
$f = "C:\ProgramData\ssh\administrators_authorized_keys"
# paste the public-key line into $f, then lock down its ACLs:
icacls $f /inheritance:r /grant "Administrators:F" /grant "SYSTEM:F"
```

## Find what the connecting side needs

```powershell
ipconfig                                   # the IPv4 address of the box
# the Creator MCP port (with Qt Creator running):
Get-NetTCPConnection -State Listen -OwningProcess (Get-Process qtcreator).Id |
    Select-Object LocalAddress, LocalPort
```

Send back: the **IP/hostname**, the **username**, and the **MCP port**.

## From the connecting side (the tunnel - no Windows config needed)

```bash
# forwards local 8080 to Creator's MCP port on their box, over SSH
ssh -N -L 8080:127.0.0.1:<MCP_PORT> <winuser>@<winhost>
```

Then point the MCP client at `127.0.0.1:8080`. The same SSH session (without
`-N`) also gives a shell for building `qtcreatorcdbext` and running raw `cdb`.

## Caveats

- **Reachability:** this only works if the box is reachable on port 22. On a
  corporate network that usually means a **VPN** or an internal IP. If it is
  behind NAT with no inbound access, use **Tailscale**/a VPN, or a **reverse
  tunnel** from the Windows box out to a shared jump host (`ssh -R` from their
  side).
- **Trust:** SSH plus the tunneled MCP grants shell *and* debugger control of
  the machine (effectively arbitrary code execution). Keep it on a trusted
  network/VPN, prefer key auth, and consider disabling password auth
  (`PasswordAuthentication no` in `C:\ProgramData\ssh\sshd_config`, then
  `Restart-Service sshd`).

## What this enables (and does not)

- **Enables:** iterating the Python bridge (`cdbbridge.py` reloads per debug
  session), observing call stacks/variables/stepping, and - if the MCP's
  `execute_command`/`evaluate_expression` reaches the cdb console - running the
  isolated bridge acceptance test from `cdb-reference.md` section 7 remotely.
- **Does not:** rebuild `qtcreatorcdbext` or hot-reload its DLL; C++ extension
  changes still need a local rebuild + Creator restart on the Windows box.
