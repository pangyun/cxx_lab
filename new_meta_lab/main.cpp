#include <iostream>
#include <thread>
#include "Utils.h"
#include "Resources.h"
#include "Snapshots.h"
#include "Proxy.h"

using namespace std;

int main() {
    /* auto& collections_holder = CollectionsHolder::GetInstance(); */
    /* collections_holder.Dump("-----"); */
    /* auto c1 = collections_holder.GetResource(4); */
    /* collections_holder.Dump("111111"); */

    /* cout << c1->Get()->RefCnt() << endl; */
    /* c1->Get()->Ref(); */
    /* cout << c1->Get()->RefCnt() << endl; */
    /* c1->Get()->Ref(); */
    /* cout << c1->Get()->RefCnt() << endl; */
    /* c1->Get()->UnRef(); */
    /* cout << c1->Get()->RefCnt() << endl; */
    /* c1->Get()->UnRef(); */
    /* cout << c1->Get()->RefCnt() << endl; */


    /* /1* for (auto i=0; i<100; ++i) { *1/ */
    /* /1*     collections_holder.GetResource(i); *1/ */
    /* /1* } *1/ */

    /* collections_holder.Dump(); */
    /* c1->Get()->UnRef(); */
    /* cout << c1->Get()->RefCnt() << endl; */
    /* collections_holder.Dump(); */

    SnapshotsHolder ss_holder;

    thread gc_thread(&SnapshotsHolder::BackgroundGC, &ss_holder);

    ss_holder.Add(1);
    ss_holder.Add(2);
    ss_holder.Add(3);
    ss_holder.Add(4);
    ss_holder.Add(5);

    ss_holder.NotifyDone();
    gc_thread.join();

    return 0;
}
