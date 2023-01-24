## Bootkit
![Windows](https://img.shields.io/badge/Windows-0078D6?style=for-the-badge&logo=windows&logoColor=white) ![Microsoft](https://img.shields.io/badge/Microsoft-0078D4?style=for-the-badge&logo=microsoft&logoColor=white)

malicious program designed to load as early as possible in the boot process, in order to control all stages of the operating system start up, modifying system code and drivers before anti-virus and other security components are loaded. The malicious program is loaded from the Master Boot Record (MBR) or boot sector. In effect, a bootkit is a rootkit that loads before the operating system.

The primary benefit to a bootkit infection is that it cannot be detected by standard operating systems processes because all of the components reside outside of the Windows file system.

Bootkit infections are on the decline with the increased adoption of modern operating systems and hardware utilizing UFEI and Secure Boot technologies.

## source of infection
Bootkits were historically spread via bootable floppy disks and other bootable media.  Recent bootkits may be installed using various methods, including being disguised as harmless software program and distributed alongside free downloads, or targeted to individuals as an email attachment.  Alternatively, bootkits could be installed via a malicious website utilizing vulnerabilities within the browser.  Infections that happen in this manner are usually silent and happen without any user knowledge or consent.

## Mitigation
Malwarebytes has modules to scan and remove rootkits and bootkits.
