import psutil


def read_memory() -> tuple[int, int, float]:
    vm = psutil.virtual_memory()
    return int(vm.used), int(vm.total), float(vm.percent)
