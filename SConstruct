import os, glob

AddOption('--platform',
          default='linux',
          dest='platform',
          type='string',
          nargs=1,
          action='store',
          metavar='PLATFORM',
          help='target platform (linux, mingw32, mingw32-linux)')

client_objects = ['client.o', 'common.o', 'sdl_aux.o']
server_objects = ['server.o', 'common.o']

if GetOption('platform') == 'linux':
    env = Environment(ENV={'PATH' : os.environ['PATH']})
    env['FRAMEWORKS'] = ['OpenGL', 'Foundation', 'Cocoa']
    env.Append(CPPPATH = ['/opt/local/include/'])
    env.Append(CCFLAGS='-Wall -pedantic -g -std=c99 -D_POSIX_C_SOURCE=199309L -DVERBOSE')
    env.Append(LIBPATH='.')

    env.Object(glob.glob('*.c'))

    server_libs = ['enet', 'z', 'm']
    client_libs = ['SDL', 'SDL_ttf', 'enet', 'z', 'm']

    env.Program('client', client_objects, LIBS=client_libs)
    env.Program('server', server_objects, LIBS=server_libs)
else:
    env = Environment(ENV={'PATH' : os.environ['PATH']})
    env['FRAMEWORKS'] = ['OpenGL', 'Foundation', 'Cocoa']
    env.Append(CPPPATH = ['/opt/local/include/'])
    env.Append(CCFLAGS='-Wall -pedantic -g -std=c99 -D_POSIX_C_SOURCE=199309L -DVERBOSE -DWIN32')
    env.Append(LIBPATH='.')
    if GetOption('platform') == 'mingw32-linux':
        env.Replace(CC='i486-mingw32-gcc')
    else:
        env.Replace(CC='mingw32-gcc')

    env.Object(glob.glob('*.c'))

    server_libs = ['mingw32', 'SDL', 'enet', 'z', 'm', 'ws2_32', 'winmm']
    client_libs = ['mingw32', 'SDLmain', 'SDL', 'SDL_ttf', 'enet', 'z', 'm', 'ws2_32', 'winmm']

    env.Program('client.exe', client_objects, LIBS=client_libs)
    env.Program('server.exe', server_objects, LIBS=server_libs)
