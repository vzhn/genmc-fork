/* KVM request bit set and checked without barriers: the vCPU reads the old payload */
#define BUG_NO_BARRIERS
#include "../../../../correct/classic/kvm-request/kvm-request.c"
