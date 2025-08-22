Set-PSDebug -Trace 1

curl -fssL -o 'PerforceBinaries/linux64/p4'      'https://filehost.perforce.com/perforce/r24.1/bin.linux26x86_64/p4'
curl -fssL -o 'PerforceBinaries/linux64/p4d'     'https://filehost.perforce.com/perforce/r24.1/bin.linux26x86_64/p4d'
curl -fssL -o 'PerforceBinaries/OSX/arm64/p4'    'https://filehost.perforce.com/perforce/r24.1/bin.macosx12arm64/p4'
curl -fssL -o 'PerforceBinaries/OSX/arm64/p4d'   'https://filehost.perforce.com/perforce/r24.1/bin.macosx12arm64/p4d'
curl -fssL -o 'PerforceBinaries/OSX/x86_64/p4'   'https://filehost.perforce.com/perforce/r24.1/bin.macosx1015x86_64/p4'
curl -fssL -o 'PerforceBinaries/OSX/x86_64/p4d'  'https://filehost.perforce.com/perforce/r24.1/bin.macosx1015x86_64/p4d'
curl -fssL -o 'PerforceBinaries\Win_x64\p4.exe'  'https://filehost.perforce.com/perforce/r24.1/bin.ntx64/p4.exe'
curl -fssL -o 'PerforceBinaries\Win_x64\p4d.exe' 'https://filehost.perforce.com/perforce/r24.1/bin.ntx64/p4d.exe'

Set-PSDebug -Trace 0
