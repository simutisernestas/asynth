#include "synth.cpp"

int main(void);
int main(void)
{
    Synth synth;
    if (!synth.start())
        return 1;
    synth.spin();
    synth.stop();
    return 0;
}