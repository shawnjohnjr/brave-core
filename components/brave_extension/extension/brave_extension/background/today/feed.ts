import { getOrFetchData as getOrFetchPublishers } from './publishers'
import feedToData from './feedToData'
import { URLS } from './privateCDN'
import getBraveTodayData from './feedToData'

type RemoteData = BraveToday.FeedItem[]

const feedUrl = URLS.braveTodayFeed

// TODO: db
let memoryTodayData: BraveToday.Feed | undefined

let readLock: Promise<void> | null

const getLocalDataLock = new Promise<void>(resolve => {
  // chrome.storage.local.get('today', (data) => {
  //   if (data && data.today) {
  //     memoryTodayData = data.today
  //   }
    resolve()
  // })
})

function performUpdateFeed() {
  // Sanity check
  if (readLock) {
    console.error('Asked to update feed but already waiting for another update!')
    return
  }
  // Only run this once at a time, otherwise wait for the update
  readLock = new Promise(async function (resolve, reject) {
    try {
      const [feedResponse, publishers] = await Promise.all([
        fetch(feedUrl),
        getOrFetchPublishers()
      ])
      if (feedResponse.ok) {
        const feedContents: RemoteData = await feedResponse.json()
        console.log('Got feed', feedContents)
        if (!publishers) {
          throw new Error('no publishers to filter feed')
        }
        const enabledPublishers = {}
        for (const publisher of Object.values(publishers)) {
          if (publisher.enabled){
            enabledPublishers[publisher.publisher_id] = publisher
          }
        }
        memoryTodayData = await feedToData(feedContents, enabledPublishers)
        resolve()
        // console.log('setting today feed data', memoryTodayData)
        chrome.storage.local.set({ today: memoryTodayData })
      } else {
        throw new Error(`Not ok when fetching feed. Status ${feedResponse.status} (${feedResponse.statusText})`)
      }
    } catch (e) {
      console.error('Could not process feed contents from ', feedUrl)
      reject(e)
    } finally {
      readLock = null
    }
  })
}


export async function getOrFetchData() {
  await getLocalDataLock
  if (memoryTodayData) {
    return memoryTodayData
  } else {
    return await update()
  }
}

export async function update() {
  // Fetch but only once at a time, and wait.
  if (!readLock) {
    performUpdateFeed()
  }
  await readLock
  return memoryTodayData
}
