import ctypes
import mmap
import os
import signal
import subprocess
import threading
import time

_libc = ctypes.CDLL(None, use_errno=True)

TOTAL_TAKEOFFS = 20
STRIPS = 5
SHM_NAME = "/control_shm"

shm_data = []

# TODO1: Size of shared memory for 3 integers (current process pid, radio, ground) use ctypes.sizeof()
SHM_LENGTH = ctypes.sizeof(ctypes.c_int) * 3

# Global variables and locks
planes = 0  # planes waiting
takeoffs = 0  # local takeoffs (per thread)
total_takeoffs = 0  # total takeoffs

#mutexes 
# runtime locks
runway1_lock = threading.Lock()
runway2_lock = threading.Lock()
state_lock = threading.Lock()

# will be set in main
radio_proc = None
radio_pid = 0

#ctypes prototypes

# prepare ctypes prototypes
_libc.shm_open.argtypes = [ctypes.c_char_p, ctypes.c_int, ctypes.c_int]
_libc.shm_open.restype = ctypes.c_int

_libc.ftruncate.argtypes = [ctypes.c_int, ctypes.c_size_t]
_libc.ftruncate.restype = ctypes.c_int

_libc.shm_unlink.argtypes = [ctypes.c_char_p]
_libc.shm_unlink.restype = ctypes.c_int


def create_shared_memory():
    """Create shared memory segment for PID exchange"""
    # TODO 6:
    # 1. Encode (utf-8) the shared memory name to use with shm_open
    encoded_name =  SHM_NAME.encode("utf-8")
    # 2. Temporarily adjust the permission mask (umask) so the memory can be created with appropriate permissions
    unmask = os.umask(0)
    flags = os.O_CREAT | os.O_RDWR
    mode = 0o666
    try:
           # 3. Use _libc.shm_open to create the shared memory
        _libc.shm_unlink(SHM_NAME.encode('utf-8'))
        fd = _libc.shm_open(encoded_name, flags, mode)
        if fd == -1:
            err = ctypes.get_errno()
            raise OSError(err, "shm_open failed")
        
        # 4. Use _libc.ftruncate to set the size of the shared memory (SHM_LENGTH)
        if _libc.ftruncate(fd, SHM_LENGTH) == -1:
            err = ctypes.get_errno()
            try:
                os.close(fd)
            except Exception:
                pass
            raise OSError(err, "ftruncate failed")
    finally:
        # 5. Restore the original permission mask (umask)
        os.umask(unmask)
 
    # 6. Use mmap to map the shared memory
    prot = mmap.PROT_READ | mmap.PROT_WRITE
    flags_mmap = mmap.MAP_SHARED
    mm = mmap.mmap(fd, SHM_LENGTH, flags=flags_mmap, prot=prot)
    # 7. Create an integer-array view (use memoryview()) to access the shared memory
    mv = memoryview(mm).cast('i') #apparently 'i' is C int (native)
    # 8. Return the file descriptor (shm_open), mmap object and memory view
    return fd, mm, mv


def HandleUSR2(signum, frame):
    """Handle external signal indicating arrival of 5 new planes.
    Complete function to update waiting planes"""
    global planes
    # TODO 4: increment the global variable planes
    with state_lock:
        planes += 5
    #pass


def TakeOffFunction(agent_id: int):
    """Function executed by each THREAD to control takeoffs.
    Uses runway1_lock, runway2_lock, and state_lock to synchronize operations."""
    global planes, takeoffs, total_takeoffs, radio_pid

    local_takeoffs = 0

    while True:
        # Check if total takeoffs goal reached
        with state_lock:
            if total_takeoffs >= TOTAL_TAKEOFFS:
                break

        # Try to acquire a runway
        used_runway = None
        if runway1_lock.acquire(blocking=False):
            used_runway = 1
        elif runway2_lock.acquire(blocking=False):
            used_runway = 2
        else:
            # No runway available → small delay and retry
            time.sleep(0.001)
            continue

        # We now have a runway, safely access shared state
        with state_lock:
            if total_takeoffs >= TOTAL_TAKEOFFS:
                # Release runway if we already reached the goal
                if used_runway == 1:
                    runway1_lock.release()
                else:
                    runway2_lock.release()
                break

            if planes > 0:
                # Consume one plane and update counters
                planes -= 1
                takeoffs += 1
                total_takeoffs += 1
                local_takeoffs += 1

                print(f"[Controller {agent_id}] Plane taking off on runway {used_runway} "
                      f"(Total: {total_takeoffs})")

                # Send SIGUSR1 every 5 local takeoffs
                if takeoffs == 5:
                    if radio_pid > 0:
                        try:
                            os.kill(radio_pid, signal.SIGUSR1)
                            print(f"[Controller {agent_id}] Sent SIGUSR1 to radio.")
                        except ProcessLookupError:
                            pass
                    takeoffs = 0

        # Simulate the time a takeoff takes
        time.sleep(1)

        # Release the runway
        if used_runway == 1:
            runway1_lock.release()
        else:
            runway2_lock.release()

    # Send SIGTERM when the total takeoffs target is reached
    if radio_pid > 0:
        try:
            os.kill(radio_pid, signal.SIGTERM)
            print(f"[Controller {agent_id}] Sent SIGTERM to radio (finished).")
        except ProcessLookupError:
            pass



def launch_radio():
    """unblock the SIGUSR2 signal so the child receives it"""
    def _unblock_sigusr2():
        signal.pthread_sigmask(signal.SIG_UNBLOCK, {signal.SIGUSR2})

    # TODO 8: Launch the external 'radio' process using subprocess.Popen()
    process = subprocess.Popen(["./../radio/build/radio", SHM_NAME], preexec_fn = _unblock_sigusr2)
    return process


def main():
    global shm_data, radio_proc, radio_pid, planes, takeoffs, total_takeoffs

    # TODO 2: set the handler for the SIGUSR2 signal to HandleUSR2 
    signal.signal(signal.SIGUSR2, HandleUSR2)
    # TODO 5: Create the shared memory and store the current process PID using create_shared_memory()
    fd, mm, shm_data = create_shared_memory()
    
    try:
        shm_data[0] = os.getpid()
        # TODO 7: Run radio and store its PID in shared memory, use the launch_radio function
        radio_proc = launch_radio()
        radio_pid = radio_proc.pid
        shm_data[1] = radio_pid
    
        # TODO 9: Create and start takeoff controller threads (STRIPS) 
        threads = []
        for i in range(STRIPS):
            t = threading.Thread(target=TakeOffFunction, args=(i,))
            t.start()
            threads.append(t)
        # TODO 10: Wait for all threads to finish their work
        for t in threads:
            t.join()
    # TODO 11: Release shared memory and close resources
    finally:
        # Primero, liberar cualquier memoryview
        try:
            shm_data.release()
        except Exception:
            pass
            
        # Cerrar el mmap (esto es el equivalente a munmap)
        try:
            mm.close()
        except Exception:
            pass

        # Cerrar el file descriptor
        try:
            os.close(fd)
        except Exception:
            pass

        # Eliminar el shared memory (equivalente a shm_unlink)
        try:
            _libc.shm_unlink(SHM_NAME.encode('utf-8'))
        except Exception:
            pass

        # Esperar que el proceso del radio termine
        if radio_proc is not None:
            try:
                radio_proc.wait(timeout=1)
            except Exception:
                pass



if __name__ == "__main__":
    main()
