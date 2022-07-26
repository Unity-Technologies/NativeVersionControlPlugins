# Perforce MFA

## Basic Knowledge

<https://www.perforce.com/manuals/p4sag/Content/P4SAG/security.mfa.html>

<https://www.perforce.com/manuals/p4sag/Content/P4SAG/triggers.second_factor_auth.html#Variables_for_the_three_mandatory_triggers>

Support for multi-factor authentication is provided by installing three mandatory triggers:

- auth-pre-2fa, which presents the user a list of authentication methods
- auth-init-2fa, which begins the flow for the chosen authentication method
- auth-check-2fa, which checks whether passwords are valid

## Triggers

How to create a sample trigger:
<https://www.perforce.com/manuals/p4sag/Content/P4SAG/scripting.trigger.creating.html>

```
p4 triggers
bypassad auth-check auth "/home/perforce/bypassad.sh %user%"
```

### MFA testing triggers

Download this script with a testing version of the three required triggers:
<https://github.com/perforce/helix-authentication-extension/blob/master/containers/p4d/mfa-trigger.sh>

Note: There is a custom version of this script in our local repo (NativeVersionControlPlugins/MFA) that actually works (specially for MacOs users)
- mfa-trigger.sh > bash script fot MacOs and Linux
- mfa-trigger.ps1 > powershell script for Windows

Then run:
```
p4 triggers
```
and add the following lines at the end of the form to set up the three triggers.
<https://github.com/perforce/helix-authentication-extension/blob/master/containers/p4d/triggers.txt>

```
Triggers:
 test-pre-2fa auth-pre-2fa auth "[local-path-to-mfa-trigger]/mfa-trigger.sh -t pre-2fa -e %quote%%email%%quote% -u %user% -h %host%"
 test-init-2fa auth-init-2fa auth "[local-path-to-mfa-trigger]/mfa-trigger.sh -t init-2fa -e %quote%%email%%quote% -u %user% -h %host% -m %method%"
 test-check-2fa auth-check-2fa auth "[local-path-to-mfa-trigger]/mfa-trigger.sh -t check-2fa -e %quote%%email%%quote% -u %user% -h %host% -s %scheme% -k %token%"
 ```

## Test MFA: Step by step

- Configure triggers by following the steps described in MFA testing triggers section
- Restart p4d server
- Run <code>p4admin</code>
    - Server has changed the authentication level to 3, what will require you to set a password if no previously configured with at least 8 characters length (aaaa1111)
- Create a new user (mfa)
    - User: mfa
    - Full name: Multi Factor Authentication
    - Email: [your-name]+mfa@unity3d.com
    - AuthMethod: perforce+2fa
- Create a new user (noauth)
    - User: noauth
    - Full name: MFA No Authentication
    - Email: [your-name]+noauth@unity3d.com
    - AuthMethod: perforce+2fa
- Modify <code>P4USER</code> to <code>mfa</code> testing user with <code>p4 set P4USER=mfa</code>
    - You can check current client vars with <code>p4 set</code>
- Run <code>p4 login</code>
    - If you are required to set/reset the password, you can do that with <code>p4 passwd</code>
- You will be required to introduce the response to the challenge: ABBACD
    - <code>Multi factor authentication approved.</code>
- Modify <code>P4USER</code> to <code>noauth</code> testing user with <code>p4 set P4USER=noauth</code>
- Run <code>p4 login</code>
- You will be prompted with this output:
```
Beginning multi factor authentication for user 'noauth' against server at 'localhost:1666'
Second factor authentication not required
Multi factor authentication approved.
```

## Other resources

- okta-mfa.rb: <https://swarm.workshop.perforce.com/projects/perforce_software-mfa/files/main/okta/okta-mfa.rb>